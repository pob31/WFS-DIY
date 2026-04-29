#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/ParameterDirtyTracker.h"
#include "NetworkStringUtils.h"
#include <array>

class TrackingPositionFilter;

namespace WFSNetwork { class OSCLogger; }

namespace WFSNetwork
{

/**
 * Minimal MQTT 3.1.1 protocol helpers (subscribe-only).
 * Builds and parses the subset of packets needed for a subscriber client.
 */
namespace MQTTProtocol
{
    // Packet types (high nibble of first byte)
    enum PacketType : uint8_t
    {
        CONNECT     = 0x10,
        CONNACK     = 0x20,
        PUBLISH     = 0x30,
        SUBSCRIBE   = 0x82,  // includes QoS 1 flag in fixed header
        SUBACK      = 0x90,
        PINGREQ     = 0xC0,
        PINGRESP    = 0xD0,
        DISCONNECT  = 0xE0
    };

    /** Encode MQTT remaining length (variable-length encoding). */
    inline void encodeRemainingLength (juce::MemoryOutputStream& out, int length)
    {
        do
        {
            uint8_t encodedByte = static_cast<uint8_t> (length % 128);
            length /= 128;
            if (length > 0)
                encodedByte |= 0x80;
            out.writeByte (static_cast<char> (encodedByte));
        } while (length > 0);
    }

    /** Decode MQTT remaining length from a buffer. Returns bytes consumed via outIndex. */
    inline int decodeRemainingLength (const uint8_t* data, int dataSize, int& outIndex)
    {
        int multiplier = 1;
        int value = 0;
        outIndex = 0;

        do
        {
            if (outIndex >= dataSize)
                return -1;

            uint8_t encodedByte = data[outIndex++];
            value += (encodedByte & 0x7F) * multiplier;
            multiplier *= 128;

            if ((encodedByte & 0x80) == 0)
                break;

            if (multiplier > 128 * 128 * 128)
                return -1; // malformed
        } while (true);

        return value;
    }

    /** Write a UTF-8 string with 2-byte length prefix. */
    inline void writeString (juce::MemoryOutputStream& out, const juce::String& str)
    {
        auto utf8 = str.toUTF8();
        int len = static_cast<int> (utf8.sizeInBytes() - 1); // exclude null terminator
        out.writeByte (static_cast<char> ((len >> 8) & 0xFF));
        out.writeByte (static_cast<char> (len & 0xFF));
        out.write (utf8.getAddress(), static_cast<size_t> (len));
    }

    /** Read a UTF-8 string with 2-byte length prefix. Returns empty on error. */
    inline juce::String readString (const uint8_t* data, int dataSize, int& offset)
    {
        if (offset + 2 > dataSize)
            return {};

        int len = (data[offset] << 8) | data[offset + 1];
        offset += 2;

        if (offset + len > dataSize)
            return {};

        juce::String result = WFSNetwork::safeStringFromBytes (reinterpret_cast<const char*> (data + offset), len);
        offset += len;
        return result;
    }

    /** Build a CONNECT packet. */
    inline juce::MemoryBlock buildConnect (const juce::String& clientId, int keepAliveSeconds = 60)
    {
        // Variable header + payload
        juce::MemoryOutputStream payload;

        // Protocol Name: "MQTT"
        writeString (payload, "MQTT");

        // Protocol Level: 4 (MQTT 3.1.1)
        payload.writeByte (4);

        // Connect Flags: Clean Session = 1, no will/user/password
        payload.writeByte (0x02);

        // Keep Alive (seconds)
        payload.writeByte (static_cast<char> ((keepAliveSeconds >> 8) & 0xFF));
        payload.writeByte (static_cast<char> (keepAliveSeconds & 0xFF));

        // Payload: Client ID
        writeString (payload, clientId);

        // Build full packet
        juce::MemoryOutputStream packet;
        packet.writeByte (static_cast<char> (CONNECT));
        encodeRemainingLength (packet, static_cast<int> (payload.getDataSize()));
        packet.write (payload.getData(), payload.getDataSize());

        return packet.getMemoryBlock();
    }

    /** Build a SUBSCRIBE packet for a single topic at QoS 0. */
    inline juce::MemoryBlock buildSubscribe (const juce::String& topic, uint16_t packetId = 1)
    {
        juce::MemoryOutputStream payload;

        // Packet Identifier (2 bytes)
        payload.writeByte (static_cast<char> ((packetId >> 8) & 0xFF));
        payload.writeByte (static_cast<char> (packetId & 0xFF));

        // Topic Filter + QoS
        writeString (payload, topic);
        payload.writeByte (0); // QoS 0

        // Build full packet
        juce::MemoryOutputStream packet;
        packet.writeByte (static_cast<char> (SUBSCRIBE));
        encodeRemainingLength (packet, static_cast<int> (payload.getDataSize()));
        packet.write (payload.getData(), payload.getDataSize());

        return packet.getMemoryBlock();
    }

    /** Build a PINGREQ packet. */
    inline juce::MemoryBlock buildPingReq()
    {
        juce::MemoryBlock block (2);
        auto* data = static_cast<uint8_t*> (block.getData());
        data[0] = PINGREQ;
        data[1] = 0;
        return block;
    }

    /** Build a DISCONNECT packet. */
    inline juce::MemoryBlock buildDisconnect()
    {
        juce::MemoryBlock block (2);
        auto* data = static_cast<uint8_t*> (block.getData());
        data[0] = DISCONNECT;
        data[1] = 0;
        return block;
    }
}

/**
 * TrackingMQTTReceiver
 *
 * MQTT tracking receiver for UWB systems (e.g., Qorvo DWM dev kit).
 * Connects to an MQTT broker via TCP, subscribes to a configurable topic,
 * parses JSON position payloads, and routes to inputs via a tag ID mapping table.
 *
 * MQTT Protocol: 3.1.1 (subscribe-only, QoS 0)
 * Default port: 1883
 * Topic example: dwm/node/+/uplink/location
 * JSON fields: configurable (default: x, y, z, quality)
 */
class TrackingMQTTReceiver : private juce::Thread
{
public:
    static constexpr int DEFAULT_PORT = 1883;
    static constexpr int MAX_TAG_SLOTS = 32;
    static constexpr int KEEPALIVE_SECONDS = 60;

    explicit TrackingMQTTReceiver (WFSValueTreeState& valueTreeState);
    ~TrackingMQTTReceiver() override;

    /**
     * Start connecting to the MQTT broker and subscribe to the topic.
     * @param host Broker hostname or IP address
     * @param port Broker port (default: 1883)
     * @param topic MQTT topic with optional + wildcard
     * @return true if connection initiated (actual connect happens on thread)
     */
    bool start (const juce::String& host, int port, const juce::String& topic);

    /** Stop and disconnect. */
    void stop();

    /** Check if receiver thread is running. */
    bool isActive() const { return isThreadRunning(); }

    /** Set the position filter for smoothing. */
    void setPositionFilter (TrackingPositionFilter* filter) { positionFilter = filter; }

    /** Set the logger for tracking data visibility. */
    void setLogger (OSCLogger* l) { logger = l; }

    /** Wire up the dirty tracker so tracking writes don't flag offset as dirty. */
    void setDirtyTracker (ParameterDirtyTracker* tracker) { dirtyTracker = tracker; }

    /** Update transformation parameters (offset, scale, flip). */
    void setTransformations (float offsetX, float offsetY, float offsetZ,
                             float scaleX, float scaleY, float scaleZ,
                             bool flipX, bool flipY, bool flipZ);

    /** Configure JSON field names for parsing position data. */
    void setJsonFieldNames (const juce::String& xKey, const juce::String& yKey,
                            const juce::String& zKey, const juce::String& qKey);

    //==========================================================================
    // Tag ID mapping (32 sequential slots, slot N = input N)
    //==========================================================================

    /** Set the tag ID for a slot (0-31). Empty string = slot unused. */
    void setTagId (int slot, const juce::String& tagId);

    /** Get the tag ID for a slot (0-31). */
    juce::String getTagId (int slot) const;

    /** Set all tag IDs from a comma-separated string. */
    void setTagIdsFromString (const juce::String& commaSeparated);

    /** Get all tag IDs as a comma-separated string. */
    juce::String getTagIdsAsString() const;

    //==========================================================================
    // Statistics
    //==========================================================================

    enum class ConnectionState { Disconnected, Connecting, Connected, Reconnecting };

    struct Statistics
    {
        int messagesReceived = 0;
        int positionsRouted = 0;
        ConnectionState connectionState = ConnectionState::Disconnected;
    };

    Statistics getStatistics() const;
    void resetStatistics();

    //==========================================================================
    // JSON key auto-discovery
    //==========================================================================

    /** Get the JSON keys discovered from the first received message. */
    juce::StringArray getDiscoveredJsonKeys() const;

    /** Callback fired (on message thread) when JSON keys are discovered. */
    std::function<void (const juce::StringArray&)> onJsonKeysDiscovered;

private:
    // Thread run loop
    void run() override;

    // MQTT protocol handling
    bool connectToBroker();
    bool sendSubscribe();
    void processIncomingData();
    bool readPacket (uint8_t& packetType, juce::MemoryBlock& payload);
    void handlePublish (const uint8_t* data, int dataSize);
    void sendPing();
    void sendDisconnect();

    // JSON parsing and routing
    void processJsonPayload (const juce::String& topic, const juce::String& payload);
    juce::String extractTagIdFromTopic (const juce::String& topic) const;
    juce::var resolveJsonKey (const juce::var& json, const juce::String& key) const;
    int findSlotForTagId (const juce::String& tagId) const;
    void routePositionToInput (int inputIndex, float x, float y, float z, float quality);

    // State
    WFSValueTreeState& state;
    juce::StreamingSocket socket;

    // Configuration
    juce::String brokerHost;
    int brokerPort = DEFAULT_PORT;
    juce::String subscribeTopic;
    int wildcardSegmentIndex = -1; // which segment of the topic contains the + wildcard

    // JSON field names (configurable)
    juce::String jsonKeyX { "x" };
    juce::String jsonKeyY { "y" };
    juce::String jsonKeyZ { "z" };
    juce::String jsonKeyQ { "quality" };
    juce::CriticalSection jsonKeyLock;

    // JSON key auto-discovery
    juce::StringArray discoveredJsonKeys;
    bool jsonKeysDiscovered = false;

    // Logger (shared, owned by OSCManager)
    OSCLogger* logger = nullptr;

    // Tag ID table (32 slots)
    std::array<juce::String, MAX_TAG_SLOTS> tagIds;
    juce::CriticalSection tagIdLock;

    // Position filter (shared, owned by OSCManager)
    TrackingPositionFilter* positionFilter = nullptr;

    // Snapshot-scope dirty tracker — wired up by OSCManager
    ParameterDirtyTracker* dirtyTracker = nullptr;

    // Thread control
    std::atomic<bool> shouldStop { false };

    // Transformation parameters
    std::atomic<float> transformOffsetX { 0.0f };
    std::atomic<float> transformOffsetY { 0.0f };
    std::atomic<float> transformOffsetZ { 0.0f };
    std::atomic<float> transformScaleX { 1.0f };
    std::atomic<float> transformScaleY { 1.0f };
    std::atomic<float> transformScaleZ { 1.0f };
    std::atomic<bool> transformFlipX { false };
    std::atomic<bool> transformFlipY { false };
    std::atomic<bool> transformFlipZ { false };

    // Statistics
    std::atomic<int> messagesReceived { 0 };
    std::atomic<int> positionsRouted { 0 };
    std::atomic<int> connectionState { static_cast<int> (ConnectionState::Disconnected) };

    // Read buffer
    static constexpr int READ_BUFFER_SIZE = 8192;
    uint8_t readBuffer[READ_BUFFER_SIZE];
    int readBufferPos = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackingMQTTReceiver)
};

} // namespace WFSNetwork
