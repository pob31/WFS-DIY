#include "TrackingMQTTReceiver.h"
#include "../DSP/TrackingPositionFilter.h"
#include "OSCLogger.h"

namespace WFSNetwork
{

//==============================================================================
// TrackingMQTTReceiver
//==============================================================================

TrackingMQTTReceiver::TrackingMQTTReceiver (WFSValueTreeState& valueTreeState)
    : Thread ("MQTT Tracking Receiver")
    , state (valueTreeState)
{
}

TrackingMQTTReceiver::~TrackingMQTTReceiver()
{
    stop();
}

bool TrackingMQTTReceiver::start (const juce::String& host, int port, const juce::String& topic)
{
    stop();

    brokerHost = host;
    brokerPort = port;
    subscribeTopic = topic;

    // Determine which segment of the topic contains the + wildcard
    wildcardSegmentIndex = -1;
    auto segments = juce::StringArray::fromTokens (topic, "/", "");
    for (int i = 0; i < segments.size(); ++i)
    {
        if (segments[i] == "+")
        {
            wildcardSegmentIndex = i;
            break;
        }
    }

    shouldStop = false;
    readBufferPos = 0;
    startThread();
    return true;
}

void TrackingMQTTReceiver::stop()
{
    if (isThreadRunning())
    {
        shouldStop = true;
        socket.close();
        stopThread (2000);
    }

    connectionState = static_cast<int> (ConnectionState::Disconnected);
}

void TrackingMQTTReceiver::setTransformations (float oX, float oY, float oZ,
                                                float sX, float sY, float sZ,
                                                bool fX, bool fY, bool fZ)
{
    transformOffsetX = oX; transformOffsetY = oY; transformOffsetZ = oZ;
    transformScaleX = sX;  transformScaleY = sY;  transformScaleZ = sZ;
    transformFlipX = fX;   transformFlipY = fY;   transformFlipZ = fZ;
}

void TrackingMQTTReceiver::setJsonFieldNames (const juce::String& xKey, const juce::String& yKey,
                                               const juce::String& zKey, const juce::String& qKey)
{
    juce::ScopedLock sl (jsonKeyLock);
    jsonKeyX = xKey;
    jsonKeyY = yKey;
    jsonKeyZ = zKey;
    jsonKeyQ = qKey;
}

void TrackingMQTTReceiver::setTagId (int slot, const juce::String& tagId)
{
    if (slot >= 0 && slot < MAX_TAG_SLOTS)
    {
        juce::ScopedLock sl (tagIdLock);
        tagIds[static_cast<size_t> (slot)] = tagId.trim().toUpperCase();
    }
}

juce::String TrackingMQTTReceiver::getTagId (int slot) const
{
    if (slot >= 0 && slot < MAX_TAG_SLOTS)
    {
        juce::ScopedLock sl (tagIdLock);
        return tagIds[static_cast<size_t> (slot)];
    }
    return {};
}

void TrackingMQTTReceiver::setTagIdsFromString (const juce::String& commaSeparated)
{
    auto tokens = juce::StringArray::fromTokens (commaSeparated, ",", "");
    juce::ScopedLock sl (tagIdLock);
    for (int i = 0; i < MAX_TAG_SLOTS; ++i)
    {
        if (i < tokens.size())
            tagIds[static_cast<size_t> (i)] = tokens[i].trim().toUpperCase();
        else
            tagIds[static_cast<size_t> (i)] = {};
    }
}

juce::String TrackingMQTTReceiver::getTagIdsAsString() const
{
    juce::ScopedLock sl (tagIdLock);
    juce::StringArray result;
    for (int i = 0; i < MAX_TAG_SLOTS; ++i)
        result.add (tagIds[static_cast<size_t> (i)]);
    return result.joinIntoString (",");
}

TrackingMQTTReceiver::Statistics TrackingMQTTReceiver::getStatistics() const
{
    return {
        messagesReceived.load(),
        positionsRouted.load(),
        static_cast<ConnectionState> (connectionState.load())
    };
}

void TrackingMQTTReceiver::resetStatistics()
{
    messagesReceived = 0;
    positionsRouted = 0;
}

juce::StringArray TrackingMQTTReceiver::getDiscoveredJsonKeys() const
{
    juce::ScopedLock sl (jsonKeyLock);
    return discoveredJsonKeys;
}

//==============================================================================
// Thread
//==============================================================================

void TrackingMQTTReceiver::run()
{
    int reconnectDelay = 1000; // Start at 1 second

    while (! shouldStop.load() && ! threadShouldExit())
    {
        connectionState = static_cast<int> (ConnectionState::Connecting);

        if (connectToBroker())
        {
            connectionState = static_cast<int> (ConnectionState::Connected);
            reconnectDelay = 1000; // Reset backoff on successful connect

            if (sendSubscribe())
            {
                // Main read loop
                auto lastPingTime = juce::Time::getMillisecondCounter();

                while (! shouldStop.load() && ! threadShouldExit() && socket.isConnected())
                {
                    // Check for incoming data
                    if (socket.waitUntilReady (true, 100)) // 100ms timeout
                    {
                        processIncomingData();
                    }

                    // Send keepalive ping
                    auto now = juce::Time::getMillisecondCounter();
                    if (now - lastPingTime > static_cast<juce::uint32> (KEEPALIVE_SECONDS * 500))
                    {
                        sendPing();
                        lastPingTime = now;
                    }
                }
            }
        }

        // Disconnected — close socket and try to reconnect
        socket.close();

        if (shouldStop.load() || threadShouldExit())
            break;

        connectionState = static_cast<int> (ConnectionState::Reconnecting);
        DBG ("TrackingMQTTReceiver: Disconnected, reconnecting in " << reconnectDelay << "ms");

        // Wait with backoff, checking shouldStop
        auto waitEnd = juce::Time::getMillisecondCounter() + static_cast<juce::uint32> (reconnectDelay);
        while (juce::Time::getMillisecondCounter() < waitEnd && ! shouldStop.load() && ! threadShouldExit())
            Thread::sleep (100);

        reconnectDelay = juce::jmin (reconnectDelay * 2, 30000); // Max 30s backoff
    }

    // Clean disconnect
    if (socket.isConnected())
    {
        sendDisconnect();
        socket.close();
    }

    connectionState = static_cast<int> (ConnectionState::Disconnected);
}

//==============================================================================
// MQTT Protocol
//==============================================================================

bool TrackingMQTTReceiver::connectToBroker()
{
    if (! socket.connect (brokerHost, brokerPort, 5000)) // 5s timeout
    {
        DBG ("TrackingMQTTReceiver: Failed to connect to " << brokerHost << ":" << brokerPort);
        return false;
    }

    // Generate a unique client ID
    juce::String clientId = "WFS-DIY-" + juce::String (juce::Random::getSystemRandom().nextInt (99999));
    auto connectPacket = MQTTProtocol::buildConnect (clientId, KEEPALIVE_SECONDS);

    if (socket.write (connectPacket.getData(), static_cast<int> (connectPacket.getSize())) < 0)
        return false;

    // Wait for CONNACK
    if (! socket.waitUntilReady (true, 5000))
        return false;

    uint8_t connackBuf[4];
    int bytesRead = socket.read (connackBuf, 4, true);

    if (bytesRead < 4)
        return false;

    // Check packet type is CONNACK (0x20) and return code is 0 (accepted)
    if ((connackBuf[0] & 0xF0) != MQTTProtocol::CONNACK)
        return false;

    if (connackBuf[3] != 0) // Return code 0 = Connection Accepted
    {
        DBG ("TrackingMQTTReceiver: CONNACK refused with code " << (int) connackBuf[3]);
        return false;
    }

    readBufferPos = 0;
    return true;
}

bool TrackingMQTTReceiver::sendSubscribe()
{
    auto subscribePacket = MQTTProtocol::buildSubscribe (subscribeTopic);

    if (socket.write (subscribePacket.getData(), static_cast<int> (subscribePacket.getSize())) < 0)
        return false;

    // Wait for SUBACK
    if (! socket.waitUntilReady (true, 5000))
        return false;

    uint8_t subackBuf[5];
    int bytesRead = socket.read (subackBuf, 5, true);

    if (bytesRead < 5)
        return false;

    if ((subackBuf[0] & 0xF0) != MQTTProtocol::SUBACK)
        return false;

    // Check return code is not 0x80 (failure)
    if (subackBuf[4] == 0x80)
    {
        DBG ("TrackingMQTTReceiver: SUBSCRIBE failed");
        return false;
    }

    return true;
}

void TrackingMQTTReceiver::processIncomingData()
{
    // Read available data into buffer
    int space = READ_BUFFER_SIZE - readBufferPos;
    if (space <= 0) { readBufferPos = 0; space = READ_BUFFER_SIZE; }

    int bytesRead = socket.read (readBuffer + readBufferPos, space, false);
    if (bytesRead <= 0)
        return;

    readBufferPos += bytesRead;

    // Process complete packets from buffer
    int offset = 0;
    while (offset < readBufferPos)
    {
        if (offset + 2 > readBufferPos)
            break; // Need at least 2 bytes (type + length)

        uint8_t packetType = readBuffer[offset] & 0xF0;
        int lengthOffset = 0;
        int remainingLength = MQTTProtocol::decodeRemainingLength (
            readBuffer + offset + 1, readBufferPos - offset - 1, lengthOffset);

        if (remainingLength < 0)
            break; // Incomplete length encoding

        int totalPacketSize = 1 + lengthOffset + remainingLength;
        if (offset + totalPacketSize > readBufferPos)
            break; // Incomplete packet

        // Process this packet
        if (packetType == MQTTProtocol::PUBLISH)
        {
            handlePublish (readBuffer + offset, totalPacketSize);
        }
        // PINGRESP: nothing to do
        // Other packets: ignore

        offset += totalPacketSize;
    }

    // Shift remaining data to start of buffer
    if (offset > 0 && offset < readBufferPos)
    {
        std::memmove (readBuffer, readBuffer + offset, static_cast<size_t> (readBufferPos - offset));
        readBufferPos -= offset;
    }
    else if (offset >= readBufferPos)
    {
        readBufferPos = 0;
    }
}

void TrackingMQTTReceiver::handlePublish (const uint8_t* data, int dataSize)
{
    ++messagesReceived;

    // Parse PUBLISH packet
    // Fixed header: type (1 byte) + remaining length (variable)
    uint8_t fixedHeader = data[0];
    int offset = 1;
    int lengthBytes = 0;
    int remainingLength = MQTTProtocol::decodeRemainingLength (data + 1, dataSize - 1, lengthBytes);
    offset += lengthBytes;

    if (remainingLength < 0 || offset + remainingLength > dataSize)
        return;

    const uint8_t* varData = data + offset;
    int varOffset = 0;

    // Read topic name
    juce::String topic = MQTTProtocol::readString (varData, remainingLength, varOffset);
    if (topic.isEmpty())
        return;

    // QoS 1 or 2 have a packet identifier (2 bytes) — skip if present
    int qos = (fixedHeader >> 1) & 0x03;
    if (qos > 0)
        varOffset += 2; // skip packet ID

    // Remaining bytes are the payload
    if (varOffset >= remainingLength)
        return;

    juce::String payload (reinterpret_cast<const char*> (varData + varOffset),
                          static_cast<size_t> (remainingLength - varOffset));

    processJsonPayload (topic, payload);
}

void TrackingMQTTReceiver::sendPing()
{
    auto ping = MQTTProtocol::buildPingReq();
    socket.write (ping.getData(), static_cast<int> (ping.getSize()));
}

void TrackingMQTTReceiver::sendDisconnect()
{
    auto disc = MQTTProtocol::buildDisconnect();
    socket.write (disc.getData(), static_cast<int> (disc.getSize()));
}

//==============================================================================
// JSON Processing & Routing
//==============================================================================

void TrackingMQTTReceiver::processJsonPayload (const juce::String& topic, const juce::String& payload)
{
    // Extract tag ID from topic
    juce::String tagId = extractTagIdFromTopic (topic);
    if (tagId.isEmpty())
        return;

    // Find which slot this tag ID maps to
    int slot = findSlotForTagId (tagId);
    if (slot < 0)
        return; // No matching slot configured

    // Log raw MQTT message to Network Log Window
    if (logger != nullptr && logger->getEnabled())
    {
        LogEntry entry;
        entry.timestamp = juce::Time::getCurrentTime();
        entry.direction = "Rx";
        entry.protocol = Protocol::MQTT;
        entry.transport = ConnectionMode::TCP;
        entry.ipAddress = brokerHost;
        entry.port = brokerPort;
        entry.address = topic;
        entry.arguments = payload.substring (0, 200);
        logger->logEntry (entry);
    }

    // Parse JSON
    auto json = juce::JSON::parse (payload);
    if (! json.isObject())
        return;

    // Auto-discover JSON keys from first message
    if (! jsonKeysDiscovered)
    {
        juce::StringArray keys;
        if (auto* obj = json.getDynamicObject())
        {
            for (const auto& prop : obj->getProperties())
            {
                auto& val = prop.value;
                if (val.isObject())
                {
                    // Enumerate nested keys with dot notation
                    if (auto* nested = val.getDynamicObject())
                    {
                        for (const auto& nestedProp : nested->getProperties())
                            keys.add (prop.name.toString() + "." + nestedProp.name.toString());
                    }
                }
                else
                {
                    keys.add (prop.name.toString());
                }
            }
        }

        if (keys.size() > 0)
        {
            {
                juce::ScopedLock sl (jsonKeyLock);
                discoveredJsonKeys = keys;
                jsonKeysDiscovered = true;
            }

            if (onJsonKeysDiscovered)
            {
                auto keyCopy = keys;
                auto cb = onJsonKeysDiscovered;
                juce::MessageManager::callAsync ([cb, keyCopy]() { cb (keyCopy); });
            }
        }
    }

    // Read field names (thread-safe)
    juce::String kx, ky, kz, kq;
    {
        juce::ScopedLock sl (jsonKeyLock);
        kx = jsonKeyX; ky = jsonKeyY; kz = jsonKeyZ; kq = jsonKeyQ;
    }

    // Extract position values using configurable keys
    auto vx = resolveJsonKey (json, kx);
    auto vy = resolveJsonKey (json, ky);
    auto vz = resolveJsonKey (json, kz);
    auto vq = resolveJsonKey (json, kq);

    if (! vx.isDouble() && ! vx.isInt() && ! vx.isInt64())
        return; // at minimum we need X
    if (! vy.isDouble() && ! vy.isInt() && ! vy.isInt64())
        return; // and Y

    float x = static_cast<float> (vx);
    float y = static_cast<float> (vy);
    float z = (vz.isDouble() || vz.isInt() || vz.isInt64()) ? static_cast<float> (vz) : 0.0f;

    // Quality: normalize to 0-1 range
    float quality = 1.0f;
    if (vq.isDouble() || vq.isInt() || vq.isInt64())
    {
        float rawQ = static_cast<float> (vq);
        // Heuristic: if > 1.0, treat as percentage (0-100)
        if (rawQ > 1.0f)
            quality = juce::jlimit (0.0f, 1.0f, rawQ / 100.0f);
        else
            quality = juce::jlimit (0.0f, 1.0f, rawQ);
    }

    // Apply transformations: offset → scale → flip
    x = (x + transformOffsetX.load()) * transformScaleX.load();
    y = (y + transformOffsetY.load()) * transformScaleY.load();
    z = (z + transformOffsetZ.load()) * transformScaleZ.load();

    if (transformFlipX.load()) x = -x;
    if (transformFlipY.load()) y = -y;
    if (transformFlipZ.load()) z = -z;

    // Route to input (slot index = input index)
    routePositionToInput (slot, x, y, z, quality);
}

juce::String TrackingMQTTReceiver::extractTagIdFromTopic (const juce::String& topic) const
{
    if (wildcardSegmentIndex < 0)
        return {};

    auto segments = juce::StringArray::fromTokens (topic, "/", "");
    if (wildcardSegmentIndex < segments.size())
        return segments[wildcardSegmentIndex].toUpperCase();

    return {};
}

juce::var TrackingMQTTReceiver::resolveJsonKey (const juce::var& json, const juce::String& key) const
{
    // Support dot notation for nested keys: "position.x" → json["position"]["x"]
    if (key.contains ("."))
    {
        auto parts = juce::StringArray::fromTokens (key, ".", "");
        juce::var current = json;
        for (const auto& part : parts)
        {
            if (auto* obj = current.getDynamicObject())
                current = obj->getProperty (juce::Identifier (part));
            else
                return {};
        }
        return current;
    }

    // Simple key lookup
    if (auto* obj = json.getDynamicObject())
        return obj->getProperty (juce::Identifier (key));

    return {};
}

int TrackingMQTTReceiver::findSlotForTagId (const juce::String& tagId) const
{
    juce::ScopedLock sl (tagIdLock);
    juce::String upper = tagId.toUpperCase();

    for (int i = 0; i < MAX_TAG_SLOTS; ++i)
    {
        if (tagIds[static_cast<size_t> (i)].isNotEmpty() && tagIds[static_cast<size_t> (i)] == upper)
            return i;
    }

    return -1;
}

void TrackingMQTTReceiver::routePositionToInput (int inputIndex, float x, float y, float z, float quality)
{
    auto posSection = state.getInputPositionSection (inputIndex);
    if (! posSection.isValid())
        return;

    // Check if tracking is active for this input
    bool trackingActive = posSection.getProperty (WFSParameterIDs::inputTrackingActive, false);
    if (! trackingActive)
        return;

    // Apply position filter (smoothing + jump detection)
    float fx = x, fy = y, fz = z;
    float smooth = static_cast<float> (posSection.getProperty (WFSParameterIDs::inputTrackingSmooth, 100));

    if (smooth > 0.0f && positionFilter != nullptr)
    {
        // Use inputIndex + 1 as a pseudo tracking ID for the filter
        if (! positionFilter->filterPosition (inputIndex, inputIndex + 1, fx, fy, fz,
                                              true, true, true, smooth, quality))
            return; // sample rejected
    }

    // Write filtered position to ValueTree.
    // Live tracking is transient — suppress dirty flagging in the snapshot scope.
    // Phase 5b: tag as Tracking-origin for the MCP staleness/notifications path.
    OriginTagScope originScope { OriginTag::Tracking };
    ParameterDirtyTracker::ScopedInternalWrite guard (dirtyTracker);
    posSection.setProperty (WFSParameterIDs::inputOffsetX, fx, nullptr);
    posSection.setProperty (WFSParameterIDs::inputOffsetY, fy, nullptr);
    posSection.setProperty (WFSParameterIDs::inputOffsetZ, fz, nullptr);

    ++positionsRouted;
}

} // namespace WFSNetwork
