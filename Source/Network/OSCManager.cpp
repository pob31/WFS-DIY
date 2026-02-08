#include "OSCManager.h"
#include "../Helpers/CoordinateConverter.h"

namespace WFSNetwork
{

// Helper: convert juce::var to float, handling both int and double types.
// XML loading may store numeric values as int (e.g. "3" -> var(int)),
// so var::isDouble() returns false even for valid numeric values.
static float varToFloat (const juce::var& v, float defaultVal = 0.0f)
{
    if (v.isVoid()) return defaultVal;
    return static_cast<float> (static_cast<double> (v));
}

//==============================================================================
// Construction / Destruction
//==============================================================================

OSCManager::OSCManager(WFSValueTreeState& valueTreeState)
    : state(valueTreeState)
    , rateLimiter(MAX_RATE_HZ)
    , logger(1000)
{
    // Initialize target statuses
    targetStatuses.fill(ConnectionStatus::Disconnected);

    // Register as ValueTree listener
    state.addListener(this);

    // Set up rate limiter callback
    rateLimiter.setSendCallback([this](int targetIndex, const juce::OSCMessage& message)
    {
        if (targetIndex >= 0 && targetIndex < MAX_TARGETS)
        {
            if (connections[static_cast<size_t>(targetIndex)])
            {
                if (connections[static_cast<size_t>(targetIndex)]->send(message))
                {
                    ++messagesSent;
                    const auto& config = targetConfigs[static_cast<size_t>(targetIndex)];
                    logger.logSentWithDetails(targetIndex, message, config.protocol,
                                              config.ipAddress, config.port, config.mode);
                }
            }
        }
    });

    // Create connections
    for (size_t i = 0; i < MAX_TARGETS; ++i)
    {
        connections[i] = std::make_unique<OSCConnection>(static_cast<int>(i));
    }

    // Start status polling timer
    startTimer(500);  // Check connection status every 500ms
}

OSCManager::~OSCManager()
{
    stopTimer();
    stopListening();
    disconnectAll();
    state.removeListener(this);
}

//==============================================================================
// Configuration
//==============================================================================

void OSCManager::applyGlobalConfig(const GlobalConfig& config)
{
    const juce::ScopedLock sl(configLock);

    bool portChanged = (config.udpReceivePort != globalConfig.udpReceivePort ||
                        config.tcpReceivePort != globalConfig.tcpReceivePort);

    globalConfig = config;
    ipFilteringEnabled = config.ipFilteringEnabled;

    DBG("OSCManager::applyGlobalConfig - IP filtering: " << (ipFilteringEnabled ? "ON" : "OFF")
        << ", allowed IPs: " << config.allowedIPs.joinIntoString(", "));

    if (portChanged && listening)
    {
        stopListening();
        startListening();
    }
}

void OSCManager::applyTargetConfig(int targetIndex, const TargetConfig& config)
{
    if (targetIndex < 0 || targetIndex >= MAX_TARGETS)
        return;

    DBG("OSCManager::applyTargetConfig - target " << targetIndex
        << " ip=" << config.ipAddress << " port=" << config.port
        << " protocol=" << static_cast<int>(config.protocol)
        << " txEnabled=" << (config.txEnabled ? "yes" : "no"));

    const juce::ScopedLock sl(configLock);

    auto& oldConfig = targetConfigs[static_cast<size_t>(targetIndex)];

    // Determine if we should be connected
    bool shouldBeConnected = (config.protocol != Protocol::Disabled && config.txEnabled);
    bool wasConnected = (oldConfig.protocol != Protocol::Disabled && oldConfig.txEnabled);

    // Check if connection parameters changed
    bool connectionParamsChanged = (config.ipAddress != oldConfig.ipAddress ||
                                    config.port != oldConfig.port ||
                                    config.mode != oldConfig.mode);

    oldConfig = config;

    // Handle disconnection cases
    if (!shouldBeConnected && wasConnected)
    {
        // Tx turned off or protocol disabled - disconnect
        DBG("OSCManager::applyTargetConfig - target " << targetIndex << " disconnecting (tx off or protocol disabled)");
        disconnectTarget(targetIndex);
    }
    else if (shouldBeConnected)
    {
        if (connectionParamsChanged || !wasConnected)
        {
            // Need to (re)connect
            if (wasConnected)
            {
                DBG("OSCManager::applyTargetConfig - target " << targetIndex << " reconnecting (params changed)");
                disconnectTarget(targetIndex);
            }
            else
            {
                DBG("OSCManager::applyTargetConfig - target " << targetIndex << " connecting");
            }
            connectTarget(targetIndex);
        }
    }
}

TargetConfig OSCManager::getTargetConfig(int targetIndex) const
{
    if (targetIndex < 0 || targetIndex >= MAX_TARGETS)
        return {};

    const juce::ScopedLock sl(configLock);
    return targetConfigs[static_cast<size_t>(targetIndex)];
}

GlobalConfig OSCManager::getGlobalConfig() const
{
    const juce::ScopedLock sl(configLock);
    return globalConfig;
}

//==============================================================================
// Connection Control
//==============================================================================

bool OSCManager::startListening()
{
    const juce::ScopedLock sl(configLock);

    if (listening)
        return true;

    // Create and configure UDP receiver (custom implementation with sender IP)
    udpReceiver = std::make_unique<OSCReceiverWithSenderIP>();
    if (!udpReceiver->connect(globalConfig.udpReceivePort))
    {
        DBG("Failed to bind UDP receiver to port " << globalConfig.udpReceivePort);
        udpReceiver.reset();
        return false;
    }
    udpReceiver->addListener(&udpListener);

    // Create and configure TCP receiver
    tcpReceiver = std::make_unique<OSCTCPReceiver>();
    if (!tcpReceiver->connect(globalConfig.tcpReceivePort))
    {
        DBG("Failed to bind TCP receiver to port " << globalConfig.tcpReceivePort);
        // TCP failure is not fatal - UDP is the primary protocol
    }
    else
    {
        tcpReceiver->addListener(&tcpListener);
    }

    listening = true;
    logger.logText("Started listening on UDP port " + juce::String(globalConfig.udpReceivePort) +
                   " and TCP port " + juce::String(globalConfig.tcpReceivePort));

    return true;
}

void OSCManager::stopListening()
{
    const juce::ScopedLock sl(configLock);

    if (!listening)
        return;

    if (udpReceiver)
    {
        udpReceiver->removeListener(&udpListener);
        udpReceiver->disconnect();
        udpReceiver.reset();
    }

    if (tcpReceiver)
    {
        tcpReceiver->removeListener(&tcpListener);
        tcpReceiver->disconnect();
        tcpReceiver.reset();
    }

    listening = false;
    logger.logText("Stopped listening on UDP and TCP");
}

bool OSCManager::connectTarget(int targetIndex)
{
    if (targetIndex < 0 || targetIndex >= MAX_TARGETS)
    {
        DBG("OSCManager::connectTarget - invalid target index: " << targetIndex);
        return false;
    }

    DBG("OSCManager::connectTarget - connecting target " << targetIndex);

    const juce::ScopedLock sl(configLock);

    auto& conn = connections[static_cast<size_t>(targetIndex)];
    auto& config = targetConfigs[static_cast<size_t>(targetIndex)];

    if (!conn)
    {
        DBG("OSCManager::connectTarget - no connection object for target " << targetIndex);
        return false;
    }

    DBG("OSCManager::connectTarget - configuring connection to " << config.ipAddress << ":" << config.port);
    conn->configure(config);

    // Set up callback for async TCP connections
    conn->onStatusChanged = [this, targetIndex, ipAddr = config.ipAddress, port = config.port](ConnectionStatus newStatus)
    {
        updateTargetStatus(targetIndex, newStatus);
        if (newStatus == ConnectionStatus::Connected)
        {
            logger.logText("Connected to target " + juce::String(targetIndex + 1) +
                           " (" + ipAddr + ":" + juce::String(port) + ")");
        }
        else if (newStatus == ConnectionStatus::Error)
        {
            logger.logText("Failed to connect to target " + juce::String(targetIndex + 1) +
                           " (" + ipAddr + ":" + juce::String(port) + ")");
        }
    };

    // For TCP, connect() returns true immediately and status comes via callback
    // For UDP, connect() is synchronous
    if (conn->connect())
    {
        // For UDP, we're connected immediately (except Remote which uses handshake)
        if (config.mode == ConnectionMode::UDP)
        {
            // For Remote protocol, start in Connecting state - handshake will set Connected
            if (config.protocol == Protocol::Remote)
            {
                updateTargetStatus(targetIndex, ConnectionStatus::Connecting);
                logger.logText("Connecting to Remote target " + juce::String(targetIndex + 1) +
                               " (" + config.ipAddress + ":" + juce::String(config.port) + ")");
                DBG("OSCManager::connectTarget - target " << targetIndex << " CONNECTING (Remote handshake)");
            }
            else
            {
                updateTargetStatus(targetIndex, ConnectionStatus::Connected);
                logger.logText("Connected to target " + juce::String(targetIndex + 1) +
                               " (" + config.ipAddress + ":" + juce::String(config.port) + ")");
                DBG("OSCManager::connectTarget - target " << targetIndex << " CONNECTED (UDP)");
            }
        }
        else
        {
            // TCP - set to Connecting, final status will come via callback
            updateTargetStatus(targetIndex, ConnectionStatus::Connecting);
            DBG("OSCManager::connectTarget - target " << targetIndex << " CONNECTING (TCP async)");
        }
        return true;
    }
    else
    {
        updateTargetStatus(targetIndex, ConnectionStatus::Error);
        DBG("OSCManager::connectTarget - target " << targetIndex << " connection FAILED");
        return false;
    }
}

void OSCManager::disconnectTarget(int targetIndex)
{
    if (targetIndex < 0 || targetIndex >= MAX_TARGETS)
        return;

    const juce::ScopedLock sl(configLock);

    if (connections[static_cast<size_t>(targetIndex)])
    {
        connections[static_cast<size_t>(targetIndex)]->disconnect();
        updateTargetStatus(targetIndex, ConnectionStatus::Disconnected);
    }
}

void OSCManager::disconnectAll()
{
    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        disconnectTarget(i);
    }
}

ConnectionStatus OSCManager::getTargetStatus(int targetIndex) const
{
    if (targetIndex < 0 || targetIndex >= MAX_TARGETS)
        return ConnectionStatus::Disconnected;

    return targetStatuses[static_cast<size_t>(targetIndex)];
}

//==============================================================================
// Message Sending
//==============================================================================

void OSCManager::sendMessage(int targetIndex, const juce::OSCMessage& message)
{
    if (targetIndex < 0 || targetIndex >= MAX_TARGETS)
    {
        DBG("OSCManager::sendMessage - invalid target index: " << targetIndex);
        return;
    }

    const auto& config = targetConfigs[static_cast<size_t>(targetIndex)];
    if (config.protocol == Protocol::Disabled || !config.txEnabled)
        return;

    rateLimiter.queueMessage(targetIndex, message);
}

void OSCManager::broadcastMessage(const juce::OSCMessage& message)
{
    rateLimiter.queueBroadcast(message);
}

void OSCManager::flushMessages()
{
    rateLimiter.flushAll();
}

//==============================================================================
// REMOTE Protocol
//==============================================================================

void OSCManager::setRemoteSelectedChannel(int channelId)
{
    remoteSelectedChannel = channelId;
    remoteModifiedParams.clear();
}

//==============================================================================
// IP Filtering
//==============================================================================

void OSCManager::setIPFilteringEnabled(bool enabled)
{
    ipFilteringEnabled = enabled;
}

//==============================================================================
// OSC Query
//==============================================================================

bool OSCManager::startOSCQuery(int oscPort, int httpPort)
{
    if (!oscQueryServer)
        oscQueryServer = std::make_unique<OSCQueryServer>(state);

    if (oscQueryServer->start(oscPort, httpPort))
    {
        logger.logText("OSC Query server started on HTTP port " + juce::String(httpPort));
        return true;
    }

    return false;
}

void OSCManager::stopOSCQuery()
{
    if (oscQueryServer)
    {
        oscQueryServer->stop();
        logger.logText("OSC Query server stopped");
    }
}

bool OSCManager::isOSCQueryRunning() const
{
    return oscQueryServer && oscQueryServer->isRunning();
}

//==============================================================================
// Tracking OSC
//==============================================================================

bool OSCManager::startTrackingReceiver(int port, const juce::String& pathPattern)
{
    stopTrackingReceiver();

    trackingReceiver = std::make_unique<TrackingOSCReceiver>(state);

    if (!trackingReceiver->start(port, pathPattern))
    {
        logger.logText("Tracking OSC receiver failed to start on port " + juce::String(port));
        trackingReceiver.reset();
        return false;
    }

    logger.logText("Tracking OSC receiver started on port " + juce::String(port));
    return true;
}

void OSCManager::stopTrackingReceiver()
{
    if (trackingReceiver)
    {
        trackingReceiver->stop();
        trackingReceiver.reset();
        logger.logText("Tracking OSC receiver stopped");
    }
}

bool OSCManager::isTrackingReceiverRunning() const
{
    return trackingReceiver && trackingReceiver->isActive();
}

void OSCManager::updateTrackingTransformations(float offsetX, float offsetY, float offsetZ,
                                                float scaleX, float scaleY, float scaleZ,
                                                bool flipX, bool flipY, bool flipZ)
{
    if (trackingReceiver)
    {
        trackingReceiver->setTransformations(offsetX, offsetY, offsetZ,
                                              scaleX, scaleY, scaleZ,
                                              flipX, flipY, flipZ);
    }
}

bool OSCManager::updateTrackingPathPattern(const juce::String& pathPattern)
{
    if (trackingReceiver)
    {
        return trackingReceiver->setPathPattern(pathPattern);
    }
    return false;
}

//==============================================================================
// PSN Tracking
//==============================================================================

bool OSCManager::startPSNReceiver(int port,
                                   const juce::String& networkInterface,
                                   const juce::String& multicastAddress)
{
    stopPSNReceiver();

    psnReceiver = std::make_unique<TrackingPSNReceiver>(state);

    if (!psnReceiver->start(port, networkInterface, multicastAddress))
    {
        logger.logText("PSN tracking receiver failed to start on port " + juce::String(port));
        psnReceiver.reset();
        return false;
    }

    logger.logText("PSN tracking receiver started on port " + juce::String(port)
                   + " multicast " + multicastAddress
                   + (networkInterface.isNotEmpty() ? " interface " + networkInterface : ""));
    return true;
}

void OSCManager::stopPSNReceiver()
{
    if (psnReceiver)
    {
        psnReceiver->stop();
        psnReceiver.reset();
        logger.logText("PSN tracking receiver stopped");
    }
}

bool OSCManager::isPSNReceiverRunning() const
{
    return psnReceiver && psnReceiver->isActive();
}

void OSCManager::updatePSNTransformations(float offsetX, float offsetY, float offsetZ,
                                           float scaleX, float scaleY, float scaleZ,
                                           bool flipX, bool flipY, bool flipZ)
{
    if (psnReceiver)
    {
        psnReceiver->setTransformations(offsetX, offsetY, offsetZ,
                                         scaleX, scaleY, scaleZ,
                                         flipX, flipY, flipZ);
    }
}

//==============================================================================
// RTTrP Tracking
//==============================================================================

bool OSCManager::startRTTrPReceiver(int port)
{
    stopRTTrPReceiver();

    rttrpReceiver = std::make_unique<TrackingRTTrPReceiver>(state);

    if (!rttrpReceiver->start(port))
    {
        logger.logText("RTTrP tracking receiver failed to start on port " + juce::String(port));
        rttrpReceiver.reset();
        return false;
    }

    logger.logText("RTTrP tracking receiver started on port " + juce::String(port));
    return true;
}

void OSCManager::stopRTTrPReceiver()
{
    if (rttrpReceiver)
    {
        rttrpReceiver->stop();
        rttrpReceiver.reset();
        logger.logText("RTTrP tracking receiver stopped");
    }
}

bool OSCManager::isRTTrPReceiverRunning() const
{
    return rttrpReceiver && rttrpReceiver->isActive();
}

void OSCManager::updateRTTrPTransformations(float offsetX, float offsetY, float offsetZ,
                                             float scaleX, float scaleY, float scaleZ,
                                             bool flipX, bool flipY, bool flipZ)
{
    if (rttrpReceiver)
    {
        rttrpReceiver->setTransformations(offsetX, offsetY, offsetZ,
                                           scaleX, scaleY, scaleZ,
                                           flipX, flipY, flipZ);
    }
}

//==============================================================================
// Logging
//==============================================================================

void OSCManager::setLoggingEnabled(bool enabled)
{
    logger.setEnabled(enabled);
}

//==============================================================================
// Statistics
//==============================================================================

OSCManager::Statistics OSCManager::getStatistics() const
{
    Statistics stats;
    stats.messagesSent = messagesSent.load();
    stats.messagesReceived = messagesReceived.load();
    stats.messagesCoalesced = static_cast<int>(rateLimiter.getTotalCoalesced());
    stats.parseErrors = parseErrors.load();
    return stats;
}

void OSCManager::resetStatistics()
{
    messagesSent = 0;
    messagesReceived = 0;
    parseErrors = 0;
    rateLimiter.resetStats();
}

//==============================================================================
// ValueTree::Listener
//==============================================================================

void OSCManager::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    // Check if this is a stage/config parameter that needs broadcasting to Remote
    if (property == WFSParameterIDs::stageWidth ||
        property == WFSParameterIDs::stageDepth ||
        property == WFSParameterIDs::stageHeight ||
        property == WFSParameterIDs::stageDiameter ||
        property == WFSParameterIDs::stageShape ||
        property == WFSParameterIDs::domeElevation ||
        property == WFSParameterIDs::originWidth ||
        property == WFSParameterIDs::originDepth ||
        property == WFSParameterIDs::originHeight)
    {
        sendStageConfigToRemote();
        return;  // Don't process as channel parameter
    }

    // Handle global tracking settings change - broadcast computed tracking state to all inputs
    if (property == WFSParameterIDs::trackingEnabled ||
        property == WFSParameterIDs::trackingProtocol)
    {
        sendAllTrackingStatesToRemote();
        return;  // Don't process as channel parameter
    }

    // Handle input channel count change separately - send /inputs to all connected Remote targets
    if (property == WFSParameterIDs::inputChannels)
    {
        int inputs = static_cast<int>(tree.getProperty(property));
        juce::OSCMessage inputsMsg = OSCMessageBuilder::buildConfigIntMessage("/inputs", inputs);
        for (int i = 0; i < MAX_TARGETS; ++i)
        {
            if (targetConfigs[static_cast<size_t>(i)].protocol == Protocol::Remote &&
                remoteStates[static_cast<size_t>(i)].phase == RemoteConnectionState::Phase::Connected)
            {
                sendMessage(i, inputsMsg);
            }
        }
        return;  // Don't process as channel parameter
    }

    // Handle clusterReferenceMode change - broadcast to all connected Remote targets
    if (property == WFSParameterIDs::clusterReferenceMode)
    {
        // Get cluster index from the parent Cluster tree node's id property
        int clusterId = static_cast<int>(tree.getProperty(WFSParameterIDs::id));
        int refMode = static_cast<int>(tree.getProperty(property));

        juce::OSCMessage msg("/cluster/referenceMode");
        msg.addInt32(clusterId);
        msg.addInt32(refMode);

        for (int i = 0; i < MAX_TARGETS; ++i)
        {
            if (targetConfigs[static_cast<size_t>(i)].protocol == Protocol::Remote &&
                remoteStates[static_cast<size_t>(i)].phase == RemoteConnectionState::Phase::Connected)
            {
                sendMessage(i, msg);
            }
        }
        return;  // Don't process as channel parameter
    }

    // Handle reverb algorithm parameter changes (global, no channel ID)
    if (tree.getType() == WFSParameterIDs::ReverbAlgorithm)
    {
        auto msg = OSCMessageBuilder::buildConfigMessage(property,
                       static_cast<float>(static_cast<double>(tree.getProperty(property))));
        if (msg.has_value())
        {
            for (int i = 0; i < MAX_TARGETS; ++i)
            {
                const auto& config = targetConfigs[static_cast<size_t>(i)];
                if (config.protocol == Protocol::OSC && config.txEnabled)
                {
                    if (incomingProtocol == Protocol::Disabled || config.protocol != incomingProtocol)
                        sendMessage(i, *msg);
                }
            }
        }
        return;
    }

    // Handle reverb pre-compressor / post-EQ / post-expander parameter changes (global, no channel ID)
    if (tree.getType() == WFSParameterIDs::ReverbPreComp ||
        tree.getType() == WFSParameterIDs::ReverbPostEQ ||
        tree.getType() == WFSParameterIDs::PostEQBand ||
        tree.getType() == WFSParameterIDs::ReverbPostExp)
    {
        auto msg = OSCMessageBuilder::buildConfigMessage(property,
                       static_cast<float>(static_cast<double>(tree.getProperty(property))));
        if (msg.has_value())
        {
            for (int i = 0; i < MAX_TARGETS; ++i)
            {
                const auto& config = targetConfigs[static_cast<size_t>(i)];
                if (config.protocol == Protocol::OSC && config.txEnabled)
                {
                    if (incomingProtocol == Protocol::Disabled || config.protocol != incomingProtocol)
                        sendMessage(i, *msg);
                }
            }
        }
        return;
    }

    // Determine if this is an input, output, or reverb parameter change
    juce::String typeName = tree.getType().toString();
    juce::var value = tree.getProperty(property);

    // Find channel index by traversing up to Input, Output, or Reverb parent
    int channelId = -1;
    bool isInput = false;
    bool isOutput = false;
    bool isReverb = false;

    juce::ValueTree parent = tree;
    while (parent.isValid())
    {
        if (parent.getType() == WFSParameterIDs::Input)
        {
            channelId = parent.getProperty(WFSParameterIDs::id);
            isInput = true;
            break;
        }
        else if (parent.getType() == WFSParameterIDs::Output)
        {
            channelId = parent.getProperty(WFSParameterIDs::id);
            isOutput = true;
            break;
        }
        else if (parent.getType() == WFSParameterIDs::Reverb)
        {
            channelId = parent.getProperty(WFSParameterIDs::id);
            isReverb = true;
            break;
        }
        parent = parent.getParent();
    }

    if (channelId < 0)
        return;

    // Send to appropriate targets based on protocol
    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        const auto& config = targetConfigs[static_cast<size_t>(i)];

        if (config.protocol == Protocol::Disabled || !config.txEnabled)
            continue;

        // Loop prevention: skip targets with same protocol as incoming message
        // This prevents echo loops while still allowing cross-protocol forwarding
        if (incomingProtocol != Protocol::Disabled && config.protocol == incomingProtocol)
            continue;

        if (config.protocol == Protocol::OSC)
        {
            // Standard OSC protocol
            std::optional<juce::OSCMessage> msg;

            // Check for numeric values (both int and double)
            bool isNumeric = value.isDouble() || value.isInt() || value.isInt64();

            if (isInput && isNumeric)
            {
                msg = OSCMessageBuilder::buildInputMessage(property, channelId,
                                                           static_cast<float>(static_cast<double>(value)));
            }
            else if (isOutput && isNumeric)
            {
                msg = OSCMessageBuilder::buildOutputMessage(property, channelId,
                                                            static_cast<float>(static_cast<double>(value)));
            }
            else if (isReverb && isNumeric)
            {
                msg = OSCMessageBuilder::buildReverbMessage(property, channelId,
                                                             static_cast<float>(static_cast<double>(value)));
            }

            if (msg.has_value())
                sendMessage(i, *msg);
        }
        else if (config.protocol == Protocol::Remote)
        {
            // REMOTE protocol - send position changes for any input, other params only for selected channel
            if (isInput)
            {
                // Position X/Y parameters are buffered and sent as combined XY messages for smooth movement
                bool isPositionXY = (property == WFSParameterIDs::inputPositionX ||
                                     property == WFSParameterIDs::inputPositionY);
                bool isPositionZ = (property == WFSParameterIDs::inputPositionZ);
                bool isCluster = (property == WFSParameterIDs::inputCluster);

                if (isPositionXY)
                {
                    // Buffer X/Y for combined sending - handled outside the target loop
                    // (we only need to buffer once, not per-target)
                    break;  // Exit target loop - buffering handled below
                }
                else if (isPositionZ || isCluster || channelId == remoteSelectedChannel)
                {
                    // Z position or selected channel params: send immediately
                    bool isNumeric = value.isDouble() || value.isInt() || value.isInt64();
                    if (isNumeric)
                    {
                        // Use int message for integer-typed values (e.g., cluster)
                        std::optional<juce::OSCMessage> msg;
                        if (value.isInt() || value.isInt64())
                            msg = OSCMessageBuilder::buildRemoteOutputIntMessage(
                                property, channelId, static_cast<int>(value));
                        else
                            msg = OSCMessageBuilder::buildRemoteOutputMessage(
                                property, channelId, static_cast<float>(static_cast<double>(value)));

                        if (msg.has_value())
                        {
                            sendMessage(i, *msg);
                        }
                    }
                    else if (value.isString())
                    {
                        // String parameters (e.g., inputName)
                        auto msg = OSCMessageBuilder::buildRemoteOutputStringMessage(
                            property, channelId, value.toString());

                        if (msg.has_value())
                        {
                            sendMessage(i, *msg);
                        }
                    }
                }
            }
        }
    }

    // Send combined XY position to Remote targets whenever X or Y changes
    // This ensures atomic position updates for smooth movement on Android
    if (isInput && (property == WFSParameterIDs::inputPositionX ||
                    property == WFSParameterIDs::inputPositionY))
    {
        // Check if any Remote target is connected and we're not processing incoming Remote message
        bool hasConnectedRemote = false;
        for (int i = 0; i < MAX_TARGETS; ++i)
        {
            if (targetConfigs[static_cast<size_t>(i)].protocol == Protocol::Remote &&
                targetConfigs[static_cast<size_t>(i)].txEnabled &&
                remoteStates[static_cast<size_t>(i)].phase == RemoteConnectionState::Phase::Connected &&
                incomingProtocol != Protocol::Remote)
            {
                hasConnectedRemote = true;
                break;
            }
        }

        if (hasConnectedRemote)
        {
            // Get the channel index (0-based)
            int channelIndex = channelId - 1;

            // Read BOTH current position values from state
            juce::var posXVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionX);
            juce::var posYVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionY);

            float posX = varToFloat(posXVar);
            float posY = varToFloat(posYVar);

            // Send combined XY immediately - no buffering needed since we read both values
            sendInputPositionXYToRemote(channelId, posX, posY);
        }
    }

    // If inputTrackingActive changed, also send the computed fully tracked state
    if (isInput && property == WFSParameterIDs::inputTrackingActive)
    {
        sendInputFullyTrackedState(channelId);
    }
}

//==============================================================================
// Nested Listener Implementations
//==============================================================================

void OSCManager::UDPListener::oscMessageReceived(const juce::OSCMessage& message,
                                                  const juce::String& senderIP)
{
    owner.handleIncomingMessage(message, senderIP,
                                 owner.globalConfig.udpReceivePort,
                                 ConnectionMode::UDP);
}

void OSCManager::UDPListener::oscBundleReceived(const juce::OSCBundle& bundle,
                                                 const juce::String& senderIP)
{
    owner.handleIncomingBundle(bundle, senderIP,
                                owner.globalConfig.udpReceivePort,
                                ConnectionMode::UDP);
}

void OSCManager::TCPListener::oscMessageReceived(const juce::OSCMessage& message,
                                                  const juce::String& senderIP)
{
    owner.handleIncomingMessage(message, senderIP,
                                 owner.globalConfig.tcpReceivePort,
                                 ConnectionMode::TCP);
}

void OSCManager::TCPListener::oscBundleReceived(const juce::OSCBundle& bundle,
                                                 const juce::String& senderIP)
{
    owner.handleIncomingBundle(bundle, senderIP,
                                owner.globalConfig.tcpReceivePort,
                                ConnectionMode::TCP);
}

bool OSCManager::isAllowedIP(const juce::String& senderIP) const
{
    const juce::ScopedLock sl(configLock);
    return globalConfig.allowedIPs.contains(senderIP);
}

//==============================================================================
// Timer
//==============================================================================

void OSCManager::timerCallback()
{
    auto now = juce::Time::currentTimeMillis();

    // Poll connection statuses and handle Remote handshake/heartbeat
    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        const auto& config = targetConfigs[static_cast<size_t>(i)];
        auto& remoteState = remoteStates[static_cast<size_t>(i)];

        // For non-Remote protocols, poll OSCConnection status
        if (config.protocol != Protocol::Remote)
        {
            if (connections[static_cast<size_t>(i)])
            {
                auto newStatus = connections[static_cast<size_t>(i)]->getStatus();
                if (newStatus != targetStatuses[static_cast<size_t>(i)])
                {
                    updateTargetStatus(i, newStatus);
                }
            }
        }
        // For Remote protocol, use handshake state machine for connection status
        else if (config.protocol == Protocol::Remote && config.txEnabled)
        {
            switch (remoteState.phase)
            {
                case RemoteConnectionState::Phase::Disconnected:
                    // Start connection attempt
                    sendRemotePing(i);
                    remoteState.phase = RemoteConnectionState::Phase::Connecting;
                    remoteState.lastPingSentTime = now;
                    // Update UI to show connecting
                    if (targetStatuses[static_cast<size_t>(i)] != ConnectionStatus::Connecting)
                        updateTargetStatus(i, ConnectionStatus::Connecting);
                    break;

                case RemoteConnectionState::Phase::Connecting:
                    // Retry ping every HEARTBEAT_INTERVAL_MS while connecting
                    if (now - remoteState.lastPingSentTime >= HEARTBEAT_INTERVAL_MS)
                    {
                        sendRemotePing(i);
                        remoteState.lastPingSentTime = now;
                    }
                    // Ensure UI shows connecting
                    if (targetStatuses[static_cast<size_t>(i)] != ConnectionStatus::Connecting)
                        updateTargetStatus(i, ConnectionStatus::Connecting);
                    break;

                case RemoteConnectionState::Phase::Connected:
                    // Check for timeout (no pong received in CONNECTION_TIMEOUT_MS)
                    if (now - remoteState.lastPongReceivedTime >= CONNECTION_TIMEOUT_MS)
                    {
                        // Timeout - disconnect and try to reconnect
                        onRemoteDisconnected(i);
                        sendRemotePing(i);
                        remoteState.phase = RemoteConnectionState::Phase::Connecting;
                        remoteState.lastPingSentTime = now;
                    }
                    // Send heartbeat every HEARTBEAT_INTERVAL_MS
                    else if (now - remoteState.lastPingSentTime >= HEARTBEAT_INTERVAL_MS)
                    {
                        sendRemoteHeartbeat(i);
                        remoteState.lastPingSentTime = now;
                    }
                    // Ensure UI shows connected
                    if (targetStatuses[static_cast<size_t>(i)] != ConnectionStatus::Connected)
                        updateTargetStatus(i, ConnectionStatus::Connected);
                    break;
            }
        }
        else if (config.protocol == Protocol::Remote && !config.txEnabled)
        {
            // TX disabled - if we were connected, disconnect
            if (remoteState.phase != RemoteConnectionState::Phase::Disconnected)
            {
                onRemoteDisconnected(i);
            }
            // Ensure UI shows disconnected
            if (targetStatuses[static_cast<size_t>(i)] != ConnectionStatus::Disconnected)
                updateTargetStatus(i, ConnectionStatus::Disconnected);
        }
    }
}

//==============================================================================
// Internal Methods
//==============================================================================

void OSCManager::handleIncomingMessage(const juce::OSCMessage& message,
                                       const juce::String& senderIP,
                                       int port,
                                       ConnectionMode transport)
{
    // Check IP filtering before processing
    if (ipFilteringEnabled && !isAllowedIP(senderIP))
    {
        DBG("OSCManager: Blocked message from " << senderIP << " (not in allowed list)");
        logger.logRejected(message.getAddressPattern().toString(),
                          senderIP, port, transport, "IP not in allowed list");
        return;
    }

    ++messagesReceived;

    juce::String address = message.getAddressPattern().toString();

    // Determine protocol from address
    Protocol protocol = Protocol::OSC;
    if (address.startsWith("/remoteInput/"))
        protocol = Protocol::Remote;

    // Log incoming message with full details
    logger.logReceivedWithDetails(message, protocol, senderIP, port, transport);

    // Route to appropriate handler
    if (OSCMessageRouter::isInputAddress(address) || OSCMessageRouter::isOutputAddress(address)
        || OSCMessageRouter::isReverbAddress(address) || OSCMessageRouter::isConfigAddress(address))
    {
        handleStandardOSCMessage(message);
    }
    else if (OSCMessageRouter::isRemoteInputAddress(address))
    {
        handleRemoteInputMessage(message);
    }
    else if (OSCMessageRouter::isArrayAdjustAddress(address))
    {
        handleArrayAdjustMessage(message);
    }
    else if (OSCMessageRouter::isClusterMoveAddress(address))
    {
        handleClusterMoveMessage(message);
    }
    else if (OSCMessageRouter::isClusterScaleRotationAddress(address))
    {
        handleClusterScaleRotationMessage(message);
    }
    // Handle Remote handshake/heartbeat responses
    else if (address == "/remote/pong")
    {
        int targetIndex = findRemoteTargetByIP(senderIP);
        if (targetIndex >= 0 && message.size() >= 1 && message[0].isInt32())
        {
            int seqNum = message[0].getInt32();
            handleRemotePong(targetIndex, seqNum);
        }
    }
    else if (address == "/remote/heartbeatAck")
    {
        int targetIndex = findRemoteTargetByIP(senderIP);
        if (targetIndex >= 0 && message.size() >= 1 && message[0].isInt32())
        {
            int seqNum = message[0].getInt32();
            handleRemoteHeartbeatAck(targetIndex, seqNum);
        }
    }
}

void OSCManager::handleIncomingBundle(const juce::OSCBundle& bundle,
                                      const juce::String& senderIP,
                                      int port,
                                      ConnectionMode transport)
{
    // Check IP filtering before processing
    if (ipFilteringEnabled && !isAllowedIP(senderIP))
    {
        DBG("OSCManager: Blocked bundle from " << senderIP << " (not in allowed list)");
        logger.logRejected("[bundle]", senderIP, port, transport, "IP not in allowed list");
        return;
    }

    for (const auto& element : bundle)
    {
        if (element.isMessage())
        {
            // For messages within a bundle, IP already validated
            ++messagesReceived;

            const auto& message = element.getMessage();
            juce::String address = message.getAddressPattern().toString();

            Protocol protocol = Protocol::OSC;
            if (address.startsWith("/remoteInput/"))
                protocol = Protocol::Remote;

            logger.logReceivedWithDetails(message, protocol, senderIP, port, transport);

            if (OSCMessageRouter::isInputAddress(address) || OSCMessageRouter::isOutputAddress(address)
                || OSCMessageRouter::isReverbAddress(address) || OSCMessageRouter::isConfigAddress(address))
            {
                handleStandardOSCMessage(message);
            }
            else if (OSCMessageRouter::isRemoteInputAddress(address))
            {
                handleRemoteInputMessage(message);
            }
            else if (OSCMessageRouter::isArrayAdjustAddress(address))
            {
                handleArrayAdjustMessage(message);
            }
            else if (OSCMessageRouter::isClusterMoveAddress(address))
            {
                handleClusterMoveMessage(message);
            }
            else if (OSCMessageRouter::isClusterScaleRotationAddress(address))
            {
                handleClusterScaleRotationMessage(message);
            }
        }
        else if (element.isBundle())
        {
            // Recursive call for nested bundles - IP already validated
            handleIncomingBundle(element.getBundle(), senderIP, port, transport);
        }
    }
}

void OSCManager::handleStandardOSCMessage(const juce::OSCMessage& message)
{
    juce::String address = message.getAddressPattern().toString();

    //==========================================================================
    // Handle Cylindrical/Spherical coordinate addresses
    // These convert to Cartesian internally before storage
    //==========================================================================
    if (address.startsWith("/wfs/input/position") || address.startsWith("/wfs/input/offset"))
    {
        juce::String paramName = OSCMessageRouter::extractParamName(address);
        bool isPolarCoord = (paramName == "positionR" || paramName == "positionTheta" ||
                             paramName == "positionRsph" || paramName == "positionPhi" ||
                             paramName == "offsetR" || paramName == "offsetTheta" ||
                             paramName == "offsetRsph" || paramName == "offsetPhi");

        if (isPolarCoord && message.size() >= 2)
        {
            int channelId = OSCMessageRouter::extractInt(message[0]);
            float newValue = OSCMessageRouter::extractFloat(message[1]);
            int channelIndex = channelId - 1;

            if (channelIndex >= 0)
            {
                juce::MessageManager::callAsync([this, paramName, channelIndex, newValue]()
                {
                    incomingProtocol = Protocol::OSC;

                    bool isOffset = paramName.startsWith("offset");

                    // Get current Cartesian position or offset
                    auto xId = isOffset ? WFSParameterIDs::inputOffsetX : WFSParameterIDs::inputPositionX;
                    auto yId = isOffset ? WFSParameterIDs::inputOffsetY : WFSParameterIDs::inputPositionY;
                    auto zId = isOffset ? WFSParameterIDs::inputOffsetZ : WFSParameterIDs::inputPositionZ;

                    juce::var xVar = state.getInputParameter(channelIndex, xId);
                    juce::var yVar = state.getInputParameter(channelIndex, yId);
                    juce::var zVar = state.getInputParameter(channelIndex, zId);

                    float x = varToFloat(xVar);
                    float y = varToFloat(yVar);
                    float z = varToFloat(zVar);

                    // Handle cylindrical coordinates (R, theta in XY plane, Z unchanged)
                    if (paramName == "positionR" || paramName == "offsetR")
                    {
                        // Convert current to cylindrical, update R, convert back
                        auto cyl = WFSCoordinates::cartesianToCylindrical({x, y, z});
                        cyl.r = std::abs(newValue);  // Radius must be positive
                        auto cart = WFSCoordinates::cylindricalToCartesian(cyl);
                        x = cart.x;
                        y = cart.y;
                        // z unchanged
                    }
                    else if (paramName == "positionTheta" || paramName == "offsetTheta")
                    {
                        // Convert current to cylindrical, update theta, convert back
                        auto cyl = WFSCoordinates::cartesianToCylindrical({x, y, z});
                        cyl.theta = WFSCoordinates::normalizeAngle(newValue);
                        auto cart = WFSCoordinates::cylindricalToCartesian(cyl);
                        x = cart.x;
                        y = cart.y;
                        // z unchanged
                    }
                    // Handle spherical coordinates (R, theta, phi in 3D)
                    else if (paramName == "positionRsph" || paramName == "offsetRsph")
                    {
                        // Convert current to spherical, update R, convert back
                        auto sph = WFSCoordinates::cartesianToSpherical({x, y, z});
                        sph.r = std::abs(newValue);  // Radius must be positive
                        auto cart = WFSCoordinates::sphericalToCartesian(sph);
                        x = cart.x;
                        y = cart.y;
                        z = cart.z;
                    }
                    else if (paramName == "positionPhi" || paramName == "offsetPhi")
                    {
                        // Convert current to spherical, update phi (elevation), convert back
                        auto sph = WFSCoordinates::cartesianToSpherical({x, y, z});
                        sph.phi = WFSCoordinates::clampElevation(newValue);
                        auto cart = WFSCoordinates::sphericalToCartesian(sph);
                        x = cart.x;
                        y = cart.y;
                        z = cart.z;
                    }

                    // Apply constraints (only for position, not offset)
                    if (!isOffset)
                    {
                        x = applyConstraintX(channelIndex, x);
                        y = applyConstraintY(channelIndex, y);
                        z = applyConstraintZ(channelIndex, z);
                        applyConstraintDistance(channelIndex, x, y, z);
                    }

                    // Save all three values
                    state.setInputParameter(channelIndex, xId, x);
                    state.setInputParameter(channelIndex, yId, y);
                    state.setInputParameter(channelIndex, zId, z);

                    // Notify for waypoint capture (path mode)
                    if (!isOffset && onRemoteWaypointCapture)
                        onRemoteWaypointCapture(channelIndex, x, y, z);

                    incomingProtocol = Protocol::Disabled;
                });
            }
            return;  // Handled - don't fall through to normal processing
        }
    }

    //==========================================================================
    // AutomOtion cylindrical/spherical coordinate OSC handling
    // These convert to Cartesian internally before storage
    //==========================================================================
    if (address.startsWith("/wfs/input/otomo"))
    {
        juce::String paramName = OSCMessageRouter::extractParamName(address);
        bool isPolarCoord = (paramName == "otomoR" || paramName == "otomoTheta" ||
                             paramName == "otomoRsph" || paramName == "otomoPhi");

        if (isPolarCoord && message.size() >= 2)
        {
            int channelId = OSCMessageRouter::extractInt(message[0]);
            float newValue = OSCMessageRouter::extractFloat(message[1]);
            int channelIndex = channelId - 1;

            if (channelIndex >= 0)
            {
                juce::MessageManager::callAsync([this, paramName, channelIndex, newValue]()
                {
                    incomingProtocol = Protocol::OSC;

                    // Get current Cartesian destination
                    juce::var xVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputOtomoX);
                    juce::var yVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputOtomoY);
                    juce::var zVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputOtomoZ);

                    float x = varToFloat(xVar);
                    float y = varToFloat(yVar);
                    float z = varToFloat(zVar);

                    // Handle cylindrical coordinates (R, theta in XY plane, Z unchanged)
                    if (paramName == "otomoR")
                    {
                        // Convert current to cylindrical, update R, convert back
                        auto cyl = WFSCoordinates::cartesianToCylindrical({x, y, z});
                        cyl.r = juce::jlimit(0.0f, 50.0f, std::abs(newValue));  // Radius 0-50m
                        auto cart = WFSCoordinates::cylindricalToCartesian(cyl);
                        x = cart.x;
                        y = cart.y;
                        // z unchanged
                    }
                    else if (paramName == "otomoTheta")
                    {
                        // Convert current to cylindrical, update theta, convert back
                        // Note: don't normalize - allow >360 for path rotations
                        auto cyl = WFSCoordinates::cartesianToCylindrical({x, y, z});
                        cyl.theta = newValue;  // Allow any angle for path rotations
                        auto cart = WFSCoordinates::cylindricalToCartesian(cyl);
                        x = cart.x;
                        y = cart.y;
                        // z unchanged
                    }
                    // Handle spherical coordinates (R, theta, phi in 3D)
                    else if (paramName == "otomoRsph")
                    {
                        // Convert current to spherical, update R, convert back
                        auto sph = WFSCoordinates::cartesianToSpherical({x, y, z});
                        sph.r = juce::jlimit(0.0f, 50.0f, std::abs(newValue));  // Radius 0-50m
                        auto cart = WFSCoordinates::sphericalToCartesian(sph);
                        x = cart.x;
                        y = cart.y;
                        z = cart.z;
                    }
                    else if (paramName == "otomoPhi")
                    {
                        // Convert current to spherical, update phi (elevation), convert back
                        // Note: don't clamp - allow any angle for path rotations
                        auto sph = WFSCoordinates::cartesianToSpherical({x, y, z});
                        sph.phi = newValue;  // Allow any angle for path rotations
                        auto cart = WFSCoordinates::sphericalToCartesian(sph);
                        x = cart.x;
                        y = cart.y;
                        z = cart.z;
                    }

                    // Save all three values
                    state.setInputParameter(channelIndex, WFSParameterIDs::inputOtomoX, x);
                    state.setInputParameter(channelIndex, WFSParameterIDs::inputOtomoY, y);
                    state.setInputParameter(channelIndex, WFSParameterIDs::inputOtomoZ, z);

                    incomingProtocol = Protocol::Disabled;
                });
            }
            return;  // Handled - don't fall through to normal processing
        }
    }

    if (OSCMessageRouter::isInputAddress(address))
    {
        auto parsed = OSCMessageRouter::parseInputMessage(message);
        if (parsed.valid)
        {
            // Update parameter in ValueTree (will be on message thread)
            // Set flag to prevent loop (don't re-send what we just received)
            juce::MessageManager::callAsync([this, parsed]()
            {
                incomingProtocol = Protocol::OSC;  // Flag: processing incoming OSC
                // OSC uses 1-based channel IDs, but internal API uses 0-based
                int channelIndex = parsed.channelId - 1;
                if (channelIndex >= 0)
                {
                    juce::var valueToSet = parsed.value;

                    // Apply constraints to position parameters
                    if (parsed.value.isDouble() &&
                        (parsed.paramId == WFSParameterIDs::inputPositionX ||
                         parsed.paramId == WFSParameterIDs::inputPositionY ||
                         parsed.paramId == WFSParameterIDs::inputPositionZ))
                    {
                        float floatValue = static_cast<float>(static_cast<double>(parsed.value));

                        if (parsed.paramId == WFSParameterIDs::inputPositionX)
                            floatValue = applyConstraintX(channelIndex, floatValue);
                        else if (parsed.paramId == WFSParameterIDs::inputPositionY)
                            floatValue = applyConstraintY(channelIndex, floatValue);
                        else if (parsed.paramId == WFSParameterIDs::inputPositionZ)
                            floatValue = applyConstraintZ(channelIndex, floatValue);

                        // Get current position values and apply distance constraint
                        juce::var xVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionX);
                        juce::var yVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionY);
                        juce::var zVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionZ);
                        float x = varToFloat(xVar);
                        float y = varToFloat(yVar);
                        float z = varToFloat(zVar);

                        // Update with new value being set
                        if (parsed.paramId == WFSParameterIDs::inputPositionX)
                            x = floatValue;
                        else if (parsed.paramId == WFSParameterIDs::inputPositionY)
                            y = floatValue;
                        else if (parsed.paramId == WFSParameterIDs::inputPositionZ)
                            z = floatValue;

                        // Apply distance constraint (modifies x, y, z in place)
                        applyConstraintDistance(channelIndex, x, y, z);

                        // Set all position values
                        state.setInputParameter(channelIndex, WFSParameterIDs::inputPositionX, x);
                        state.setInputParameter(channelIndex, WFSParameterIDs::inputPositionY, y);
                        state.setInputParameter(channelIndex, WFSParameterIDs::inputPositionZ, z);

                        // Notify for waypoint capture (path mode)
                        if (onRemoteWaypointCapture)
                            onRemoteWaypointCapture(channelIndex, x, y, z);
                    }
                    else if (parsed.value.isDouble() || parsed.value.isString())
                    {
                        state.setInputParameter(channelIndex, parsed.paramId, valueToSet);
                    }
                }
                incomingProtocol = Protocol::Disabled;  // Clear flag
            });
        }
        else
        {
            ++parseErrors;
        }
    }
    else if (OSCMessageRouter::isOutputAddress(address))
    {
        auto parsed = OSCMessageRouter::parseOutputMessage(message);
        if (parsed.valid)
        {
            juce::MessageManager::callAsync([this, parsed]()
            {
                incomingProtocol = Protocol::OSC;  // Flag: processing incoming OSC
                // OSC uses 1-based channel IDs, but internal API uses 0-based
                int channelIndex = parsed.channelId - 1;
                if (channelIndex >= 0)
                {
                    if (parsed.value.isDouble())
                    {
                        state.setOutputParameter(channelIndex, parsed.paramId, parsed.value);
                    }
                    else if (parsed.value.isString())
                    {
                        state.setOutputParameter(channelIndex, parsed.paramId, parsed.value);
                    }
                }
                incomingProtocol = Protocol::Disabled;  // Clear flag
            });
        }
        else
        {
            ++parseErrors;
        }
    }
    else if (OSCMessageRouter::isReverbAddress(address))
    {
        auto parsed = OSCMessageRouter::parseReverbMessage(message);
        if (parsed.valid)
        {
            juce::MessageManager::callAsync([this, parsed]()
            {
                incomingProtocol = Protocol::OSC;  // Flag: processing incoming OSC
                // OSC uses 1-based channel IDs, but internal API uses 0-based
                int channelIndex = parsed.channelId - 1;
                if (channelIndex >= 0)
                {
                    if (parsed.isEQparam)
                    {
                        // EQ parameters need band index handling
                        // Get EQ band section and set the property there
                        auto reverbState = state.getReverbState(channelIndex);
                        if (reverbState.isValid())
                        {
                            auto eqSection = reverbState.getChildWithName(WFSParameterIDs::EQ);
                            if (eqSection.isValid() && parsed.bandIndex >= 1 && parsed.bandIndex <= 4)
                            {
                                auto bandSection = eqSection.getChildWithName(
                                    juce::Identifier("Band" + juce::String(parsed.bandIndex)));
                                if (bandSection.isValid())
                                {
                                    bandSection.setProperty(parsed.paramId, parsed.value, state.getUndoManager());
                                }
                            }
                        }
                    }
                    else
                    {
                        // Standard reverb parameters
                        state.setReverbParameter(channelIndex, parsed.paramId, parsed.value);
                    }
                }
                incomingProtocol = Protocol::Disabled;  // Clear flag
            });
        }
        else
        {
            ++parseErrors;
        }
    }
    else if (OSCMessageRouter::isConfigAddress(address))
    {
        auto parsed = OSCMessageRouter::parseConfigMessage(message);
        if (parsed.valid)
        {
            juce::MessageManager::callAsync([this, parsed]()
            {
                incomingProtocol = Protocol::OSC;

                // Check if this is a reverb algorithm parameter (stored in ReverbAlgorithm section)
                auto algoSection = state.ensureReverbAlgorithmSection();
                if (algoSection.isValid() && algoSection.hasProperty(parsed.paramId))
                {
                    algoSection.setProperty(parsed.paramId, parsed.value, state.getUndoManager());
                }
                // Check if this is a reverb pre-compressor parameter (stored in ReverbPreComp section)
                else if (parsed.paramId == WFSParameterIDs::reverbPreCompBypass ||
                         parsed.paramId == WFSParameterIDs::reverbPreCompThreshold ||
                         parsed.paramId == WFSParameterIDs::reverbPreCompRatio ||
                         parsed.paramId == WFSParameterIDs::reverbPreCompAttack ||
                         parsed.paramId == WFSParameterIDs::reverbPreCompRelease)
                {
                    auto preComp = state.ensureReverbPreCompSection();
                    if (preComp.isValid())
                        preComp.setProperty(parsed.paramId, parsed.value, state.getUndoManager());
                }
                // Check if this is a reverb post-EQ parameter (stored in ReverbPostEQ section)
                else if (parsed.paramId == WFSParameterIDs::reverbPostEQenable)
                {
                    auto postEQ = state.ensureReverbPostEQSection();
                    if (postEQ.isValid())
                        postEQ.setProperty(parsed.paramId, parsed.value, state.getUndoManager());
                }
                else if (parsed.paramId == WFSParameterIDs::reverbPostEQshape ||
                         parsed.paramId == WFSParameterIDs::reverbPostEQfreq ||
                         parsed.paramId == WFSParameterIDs::reverbPostEQgain ||
                         parsed.paramId == WFSParameterIDs::reverbPostEQq ||
                         parsed.paramId == WFSParameterIDs::reverbPostEQslope)
                {
                    // PostEQ band params — for now apply to all bands (band ID not in OSC message)
                    auto postEQ = state.ensureReverbPostEQSection();
                    if (postEQ.isValid())
                    {
                        for (int b = 0; b < postEQ.getNumChildren(); ++b)
                        {
                            auto band = postEQ.getChild(b);
                            if (band.isValid())
                                band.setProperty(parsed.paramId, parsed.value, state.getUndoManager());
                        }
                    }
                }
                // Check if this is a reverb post-expander parameter (stored in ReverbPostExp section)
                else if (parsed.paramId == WFSParameterIDs::reverbPostExpBypass ||
                         parsed.paramId == WFSParameterIDs::reverbPostExpThreshold ||
                         parsed.paramId == WFSParameterIDs::reverbPostExpRatio ||
                         parsed.paramId == WFSParameterIDs::reverbPostExpAttack ||
                         parsed.paramId == WFSParameterIDs::reverbPostExpRelease)
                {
                    auto postExp = state.ensureReverbPostExpSection();
                    if (postExp.isValid())
                        postExp.setProperty(parsed.paramId, parsed.value, state.getUndoManager());
                }
                else
                {
                    // Standard config parameter
                    state.setParameter(parsed.paramId, parsed.value);
                }

                incomingProtocol = Protocol::Disabled;
            });
        }
        else
        {
            ++parseErrors;
        }
    }
}

void OSCManager::handleRemoteInputMessage(const juce::OSCMessage& message)
{
    auto parsed = OSCMessageRouter::parseRemoteInputMessage(message);

    if (!parsed.valid)
    {
        ++parseErrors;
        return;
    }

    using Type = OSCMessageRouter::ParsedRemoteInput::Type;

    switch (parsed.type)
    {
        case Type::ChannelSelect:
        {
            // Channel selection from Android app - send all params back
            juce::MessageManager::callAsync([this, channelId = parsed.channelId]()
            {
                setRemoteSelectedChannel(channelId);

                if (onRemoteChannelSelect)
                    onRemoteChannelSelect(channelId);

                // Send all parameters for this channel to REMOTE targets
                sendRemoteChannelDump(channelId);
            });
            break;
        }

        case Type::PositionDelta:
            // Legacy handler for position inc/dec (kept for backward compatibility)
            handleRemotePositionDelta(parsed);
            break;

        case Type::ParameterSet:
            // Absolute value setting
            handleRemoteParameterSet(parsed);
            break;

        case Type::ParameterDelta:
            // Incremental value change
            handleRemoteParameterDelta(parsed);
            break;

        case Type::PositionXY:
            // Combined XY position (atomic update)
            handleRemotePositionXY(parsed);
            break;
    }
}

void OSCManager::handleRemotePositionDelta(const OSCMessageRouter::ParsedRemoteInput& parsed)
{
    // Apply delta to position or offset based on tracking state
    juce::MessageManager::callAsync([this, parsed]()
    {
        incomingProtocol = Protocol::Remote;  // Flag: processing incoming REMOTE message

        // Check if tracking is active for this channel
        bool trackingActive = state.getInputParameter(parsed.channelId,
                                                       WFSParameterIDs::inputTrackingActive);

        // Determine which parameter to modify
        juce::Identifier paramId;
        if (trackingActive)
        {
            // Tracking active: modify offset
            switch (parsed.axis)
            {
                case Axis::X: paramId = WFSParameterIDs::inputOffsetX; break;
                case Axis::Y: paramId = WFSParameterIDs::inputOffsetY; break;
                case Axis::Z: paramId = WFSParameterIDs::inputOffsetZ; break;
            }
        }
        else
        {
            // Tracking not active: modify position
            switch (parsed.axis)
            {
                case Axis::X: paramId = WFSParameterIDs::inputPositionX; break;
                case Axis::Y: paramId = WFSParameterIDs::inputPositionY; break;
                case Axis::Z: paramId = WFSParameterIDs::inputPositionZ; break;
            }
        }

        // Get current value and apply delta
        float currentValue = static_cast<float>(state.getInputParameter(parsed.channelId, paramId));

        float delta = parsed.deltaValue;
        if (parsed.direction == DeltaDirection::Decrement)
            delta = -delta;

        float newValue = currentValue + delta;

        // Set the new value
        state.setInputParameter(parsed.channelId, paramId, newValue);

        incomingProtocol = Protocol::Disabled;  // Clear flag
    });
}

void OSCManager::handleRemoteParameterSet(const OSCMessageRouter::ParsedRemoteInput& parsed)
{
    // Set parameter to absolute value
    juce::MessageManager::callAsync([this, parsed]()
    {
        incomingProtocol = Protocol::Remote;  // Flag: processing incoming REMOTE message

        // Remote uses 1-based channel IDs, but internal API uses 0-based
        int channelIndex = parsed.channelId - 1;
        if (channelIndex >= 0)
        {
            juce::var valueToSet = parsed.value;

            // Apply constraints to position parameters
            if (parsed.paramId == WFSParameterIDs::inputPositionX ||
                parsed.paramId == WFSParameterIDs::inputPositionY ||
                parsed.paramId == WFSParameterIDs::inputPositionZ)
            {
                float floatValue = varToFloat(parsed.value);

                // Apply single-axis constraint
                if (parsed.paramId == WFSParameterIDs::inputPositionX)
                    floatValue = applyConstraintX(channelIndex, floatValue);
                else if (parsed.paramId == WFSParameterIDs::inputPositionY)
                    floatValue = applyConstraintY(channelIndex, floatValue);
                else if (parsed.paramId == WFSParameterIDs::inputPositionZ)
                    floatValue = applyConstraintZ(channelIndex, floatValue);

                // Check if distance constraint is enabled for this channel
                juce::var coordModeVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputCoordinateMode);
                int coordMode = coordModeVar.isInt() ? static_cast<int>(coordModeVar) : 0;
                juce::var constraintDistVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputConstraintDistance);
                int constraintDist = constraintDistVar.isInt() ? static_cast<int>(constraintDistVar) : 0;
                bool distanceConstraintActive = (coordMode == 1 || coordMode == 2) && constraintDist != 0;

                if (distanceConstraintActive)
                {
                    // Distance constraint is active - read all axes and check if constraint applies
                    juce::var xVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionX);
                    juce::var yVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionY);
                    juce::var zVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionZ);
                    float x = varToFloat(xVar);
                    float y = varToFloat(yVar);
                    float z = varToFloat(zVar);

                    // Update with new value being set
                    if (parsed.paramId == WFSParameterIDs::inputPositionX)
                        x = floatValue;
                    else if (parsed.paramId == WFSParameterIDs::inputPositionY)
                        y = floatValue;
                    else if (parsed.paramId == WFSParameterIDs::inputPositionZ)
                        z = floatValue;

                    // Store unconstrained values
                    float origX = x, origY = y, origZ = z;

                    // Apply distance constraint (modifies x, y, z in place)
                    applyConstraintDistance(channelIndex, x, y, z);

                    // Only set all 3 values if constraint actually modified position
                    bool wasConstrained = !juce::approximatelyEqual(x, origX) ||
                                          !juce::approximatelyEqual(y, origY) ||
                                          !juce::approximatelyEqual(z, origZ);

                    if (wasConstrained)
                    {
                        // Constraint modified position - set all axes
                        state.setInputParameter(channelIndex, WFSParameterIDs::inputPositionX, x);
                        state.setInputParameter(channelIndex, WFSParameterIDs::inputPositionY, y);
                        state.setInputParameter(channelIndex, WFSParameterIDs::inputPositionZ, z);
                    }
                    else
                    {
                        // No constraint modification - just set single axis (faster)
                        state.setInputParameter(channelIndex, parsed.paramId, floatValue);
                    }

                    // Notify for waypoint capture (path mode)
                    if (onRemoteWaypointCapture)
                        onRemoteWaypointCapture(channelIndex, x, y, z);
                }
                else
                {
                    // No distance constraint - just set the single axis
                    state.setInputParameter(channelIndex, parsed.paramId, floatValue);

                    // Notify for waypoint capture (path mode)
                    if (onRemoteWaypointCapture)
                    {
                        juce::var xVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionX);
                        juce::var yVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionY);
                        juce::var zVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionZ);
                        float x = varToFloat(xVar);
                        float y = varToFloat(yVar);
                        float z = varToFloat(zVar);
                        onRemoteWaypointCapture(channelIndex, x, y, z);
                    }
                }

                // Notify UI to repaint map
                if (onRemotePositionReceived)
                    onRemotePositionReceived();
            }
            else
            {
                state.setInputParameter(channelIndex, parsed.paramId, valueToSet);
            }
        }

        incomingProtocol = Protocol::Disabled;  // Clear flag
    });
}

void OSCManager::handleRemoteParameterDelta(const OSCMessageRouter::ParsedRemoteInput& parsed)
{
    // Apply delta to current parameter value
    juce::MessageManager::callAsync([this, parsed]()
    {
        incomingProtocol = Protocol::Remote;  // Flag: processing incoming REMOTE message

        // Remote uses 1-based channel IDs, but internal API uses 0-based
        int channelIndex = parsed.channelId - 1;
        if (channelIndex >= 0)
        {
            // Get current value
            juce::var currentVar = state.getInputParameter(channelIndex, parsed.paramId);
            float currentValue = 0.0f;

            if (currentVar.isDouble())
                currentValue = static_cast<float>(static_cast<double>(currentVar));
            else if (currentVar.isInt())
                currentValue = static_cast<float>(static_cast<int>(currentVar));

            // Calculate delta
            float delta = 0.0f;
            if (parsed.value.isDouble())
                delta = static_cast<float>(static_cast<double>(parsed.value));
            else if (parsed.value.isInt())
                delta = static_cast<float>(static_cast<int>(parsed.value));

            if (parsed.direction == DeltaDirection::Decrement)
                delta = -delta;

            // Apply and set new value
            float newValue = currentValue + delta;

            // Apply constraints to position parameters
            if (parsed.paramId == WFSParameterIDs::inputPositionX ||
                parsed.paramId == WFSParameterIDs::inputPositionY ||
                parsed.paramId == WFSParameterIDs::inputPositionZ)
            {
                // Apply single-axis constraint
                if (parsed.paramId == WFSParameterIDs::inputPositionX)
                    newValue = applyConstraintX(channelIndex, newValue);
                else if (parsed.paramId == WFSParameterIDs::inputPositionY)
                    newValue = applyConstraintY(channelIndex, newValue);
                else if (parsed.paramId == WFSParameterIDs::inputPositionZ)
                    newValue = applyConstraintZ(channelIndex, newValue);

                // Check if distance constraint is enabled for this channel
                juce::var coordModeVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputCoordinateMode);
                int coordMode = coordModeVar.isInt() ? static_cast<int>(coordModeVar) : 0;
                juce::var constraintDistVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputConstraintDistance);
                int constraintDist = constraintDistVar.isInt() ? static_cast<int>(constraintDistVar) : 0;
                bool distanceConstraintActive = (coordMode == 1 || coordMode == 2) && constraintDist != 0;

                if (distanceConstraintActive)
                {
                    // Distance constraint is active - read all axes and check if constraint applies
                    juce::var xVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionX);
                    juce::var yVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionY);
                    juce::var zVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionZ);
                    float x = varToFloat(xVar);
                    float y = varToFloat(yVar);
                    float z = varToFloat(zVar);

                    // Update with new value being set
                    if (parsed.paramId == WFSParameterIDs::inputPositionX)
                        x = newValue;
                    else if (parsed.paramId == WFSParameterIDs::inputPositionY)
                        y = newValue;
                    else if (parsed.paramId == WFSParameterIDs::inputPositionZ)
                        z = newValue;

                    // Store unconstrained values
                    float origX = x, origY = y, origZ = z;

                    // Apply distance constraint (modifies x, y, z in place)
                    applyConstraintDistance(channelIndex, x, y, z);

                    // Only set all 3 values if constraint actually modified position
                    bool wasConstrained = !juce::approximatelyEqual(x, origX) ||
                                          !juce::approximatelyEqual(y, origY) ||
                                          !juce::approximatelyEqual(z, origZ);

                    if (wasConstrained)
                    {
                        // Constraint modified position - set all axes
                        state.setInputParameter(channelIndex, WFSParameterIDs::inputPositionX, x);
                        state.setInputParameter(channelIndex, WFSParameterIDs::inputPositionY, y);
                        state.setInputParameter(channelIndex, WFSParameterIDs::inputPositionZ, z);
                    }
                    else
                    {
                        // No constraint modification - just set single axis (faster)
                        state.setInputParameter(channelIndex, parsed.paramId, newValue);
                    }

                    // Notify for waypoint capture (path mode)
                    if (onRemoteWaypointCapture)
                        onRemoteWaypointCapture(channelIndex, x, y, z);
                }
                else
                {
                    // No distance constraint - just set the single axis
                    state.setInputParameter(channelIndex, parsed.paramId, newValue);

                    // Notify for waypoint capture (path mode)
                    if (onRemoteWaypointCapture)
                    {
                        juce::var xVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionX);
                        juce::var yVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionY);
                        juce::var zVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionZ);
                        float x = varToFloat(xVar);
                        float y = varToFloat(yVar);
                        float z = varToFloat(zVar);
                        onRemoteWaypointCapture(channelIndex, x, y, z);
                    }
                }

                // Notify UI to repaint map
                if (onRemotePositionReceived)
                    onRemotePositionReceived();
            }
            else
            {
                state.setInputParameter(channelIndex, parsed.paramId, newValue);
            }
        }

        incomingProtocol = Protocol::Disabled;  // Clear flag
    });
}

void OSCManager::handleRemotePositionXY(const OSCMessageRouter::ParsedRemoteInput& parsed)
{
    // Set combined XY position atomically
    // This prevents jagged diagonal movements when speed limiting is enabled
    juce::MessageManager::callAsync([this, parsed]()
    {
        incomingProtocol = Protocol::Remote;  // Flag: processing incoming REMOTE message

        // Remote uses 1-based channel IDs, but internal API uses 0-based
        int channelIndex = parsed.channelId - 1;
        if (channelIndex >= 0)
        {
            float posX = parsed.posX;
            float posY = parsed.posY;

            // Apply single-axis constraints
            posX = applyConstraintX(channelIndex, posX);
            posY = applyConstraintY(channelIndex, posY);

            // Check if distance constraint is enabled for this channel
            juce::var coordModeVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputCoordinateMode);
            int coordMode = coordModeVar.isInt() ? static_cast<int>(coordModeVar) : 0;
            juce::var constraintDistVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputConstraintDistance);
            int constraintDist = constraintDistVar.isInt() ? static_cast<int>(constraintDistVar) : 0;
            bool distanceConstraintActive = (coordMode == 1 || coordMode == 2) && constraintDist != 0;

            // Get current Z value for constraint calculations and waypoint capture
            juce::var zVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionZ);
            float z = varToFloat(zVar);

            if (distanceConstraintActive)
            {
                // Apply distance constraint (modifies x, y, z in place)
                applyConstraintDistance(channelIndex, posX, posY, z);
            }

            // Set both X and Y atomically (and Z if modified by distance constraint)
            state.setInputParameter(channelIndex, WFSParameterIDs::inputPositionX, posX);
            state.setInputParameter(channelIndex, WFSParameterIDs::inputPositionY, posY);
            if (distanceConstraintActive)
            {
                state.setInputParameter(channelIndex, WFSParameterIDs::inputPositionZ, z);
            }

            // Notify for waypoint capture (path mode)
            if (onRemoteWaypointCapture)
                onRemoteWaypointCapture(channelIndex, posX, posY, z);

            // Notify for composite delta tracking update
            // This must be called AFTER position is set so MainComponent can compute new delta
            if (onRemotePositionXYUpdated)
                onRemotePositionXYUpdated(channelIndex, posX, posY);

            // Notify UI to repaint map
            if (onRemotePositionReceived)
                onRemotePositionReceived();
        }

        incomingProtocol = Protocol::Disabled;  // Clear flag
    });
}

void OSCManager::handleArrayAdjustMessage(const juce::OSCMessage& message)
{
    auto parsed = OSCMessageRouter::parseArrayAdjustMessage(message);

    if (!parsed.valid)
    {
        ++parseErrors;
        return;
    }

    // Apply value change to all outputs in the specified array
    juce::MessageManager::callAsync([this, parsed]()
    {
        incomingProtocol = Protocol::Remote;  // Flag: processing incoming remote message

        // Get number of configured output channels
        int numOutputs = state.getIntParameter(WFSParameterIDs::outputChannels);

        // Iterate through all outputs and adjust those matching the array ID
        for (int outputIndex = 0; outputIndex < numOutputs; ++outputIndex)
        {
            // Get the array assignment for this output (0-based index)
            juce::var arrayVar = state.getOutputParameter(outputIndex, WFSParameterIDs::outputArray);
            int outputArrayId = arrayVar.isInt() ? static_cast<int>(arrayVar) : 0;

            // Check if this output belongs to the target array (1-based from remote)
            if (outputArrayId == parsed.arrayId)
            {
                // Get current parameter value
                juce::var currentVar = state.getOutputParameter(outputIndex, parsed.paramId);
                float currentValue = 0.0f;

                if (currentVar.isDouble())
                    currentValue = static_cast<float>(static_cast<double>(currentVar));
                else if (currentVar.isInt())
                    currentValue = static_cast<float>(static_cast<int>(currentVar));

                // Apply delta and set new value
                float newValue = currentValue + parsed.valueChange;
                state.setOutputParameter(outputIndex, parsed.paramId, newValue);
            }
        }

        incomingProtocol = Protocol::Disabled;  // Clear flag
    });
}

void OSCManager::handleClusterMoveMessage(const juce::OSCMessage& message)
{
    auto parsed = OSCMessageRouter::parseClusterMoveMessage(message);

    if (!parsed.valid)
    {
        ++parseErrors;
        return;
    }

    // Move inputs in the specified cluster by delta
    juce::MessageManager::callAsync([this, parsed]()
    {
        incomingProtocol = Protocol::Remote;  // Flag: processing incoming remote message

        // Get number of configured input channels
        int numInputs = state.getIntParameter(WFSParameterIDs::inputChannels);

        // Collect updated positions to echo back to Remote after processing
        std::vector<std::tuple<int, float, float>> updatedPositions;

        // Both ClusterMove and BarycenterMove result in moving all cluster members by delta
        // For ClusterMove (First Input mode): all inputs move by delta
        // For BarycenterMove (Barycenter mode): all inputs move by delta (to shift barycenter)

        // Iterate through all inputs and adjust those in the target cluster
        for (int inputIndex = 0; inputIndex < numInputs; ++inputIndex)
        {
            // Get the cluster assignment for this input
            juce::var clusterVar = state.getInputParameter(inputIndex, WFSParameterIDs::inputCluster);
            int inputClusterId = clusterVar.isInt() ? static_cast<int>(clusterVar) : 0;

            // Check if this input belongs to the target cluster
            if (inputClusterId == parsed.clusterId)
            {
                // Get current position
                juce::var posXVar = state.getInputParameter(inputIndex, WFSParameterIDs::inputPositionX);
                juce::var posYVar = state.getInputParameter(inputIndex, WFSParameterIDs::inputPositionY);

                float currentX = varToFloat(posXVar);
                float currentY = varToFloat(posYVar);

                // Apply delta
                float newX = currentX + parsed.deltaX;
                float newY = currentY + parsed.deltaY;

                // Set new position
                state.setInputParameter(inputIndex, WFSParameterIDs::inputPositionX, newX);
                state.setInputParameter(inputIndex, WFSParameterIDs::inputPositionY, newY);

                // 1-based channelId for OSC
                updatedPositions.emplace_back(inputIndex + 1, newX, newY);
            }
        }

        incomingProtocol = Protocol::Disabled;  // Clear flag

        // Echo updated positions back to Remote targets so Android sees all members move
        for (const auto& [channelId, x, y] : updatedPositions)
            sendInputPositionXYToRemote(channelId, x, y);
    });
}

void OSCManager::handleClusterScaleRotationMessage(const juce::OSCMessage& message)
{
    auto parsed = OSCMessageRouter::parseClusterScaleRotationMessage(message);

    if (!parsed.valid)
    {
        ++parseErrors;
        return;
    }

    juce::MessageManager::callAsync([this, parsed]()
    {
        incomingProtocol = Protocol::Remote;  // Flag: processing incoming remote message

        // Get number of configured input channels
        int numInputs = state.getIntParameter(WFSParameterIDs::inputChannels);

        // First, find all inputs in this cluster and calculate reference point
        std::vector<int> clusterInputs;
        float sumX = 0.0f, sumY = 0.0f;

        for (int inputIndex = 0; inputIndex < numInputs; ++inputIndex)
        {
            juce::var clusterVar = state.getInputParameter(inputIndex, WFSParameterIDs::inputCluster);
            int inputClusterId = clusterVar.isInt() ? static_cast<int>(clusterVar) : 0;

            if (inputClusterId == parsed.clusterId)
            {
                clusterInputs.push_back(inputIndex);

                juce::var posXVar = state.getInputParameter(inputIndex, WFSParameterIDs::inputPositionX);
                juce::var posYVar = state.getInputParameter(inputIndex, WFSParameterIDs::inputPositionY);

                float x = varToFloat(posXVar);
                float y = varToFloat(posYVar);

                sumX += x;
                sumY += y;
            }
        }

        if (clusterInputs.empty())
        {
            incomingProtocol = Protocol::Disabled;
            return;
        }

        // Calculate barycenter as reference point
        float refX = sumX / static_cast<float>(clusterInputs.size());
        float refY = sumY / static_cast<float>(clusterInputs.size());

        // Collect updated positions to echo back to Remote after processing
        std::vector<std::tuple<int, float, float>> updatedPositions;

        if (parsed.type == OSCMessageRouter::ParsedClusterScaleRotationMessage::Type::Scale)
        {
            // Apply uniform scale around reference point
            float scaleFactor = parsed.value;

            for (int inputIndex : clusterInputs)
            {
                juce::var posXVar = state.getInputParameter(inputIndex, WFSParameterIDs::inputPositionX);
                juce::var posYVar = state.getInputParameter(inputIndex, WFSParameterIDs::inputPositionY);

                float x = varToFloat(posXVar);
                float y = varToFloat(posYVar);

                // Scale offset from reference point
                float newX = refX + (x - refX) * scaleFactor;
                float newY = refY + (y - refY) * scaleFactor;

                state.setInputParameter(inputIndex, WFSParameterIDs::inputPositionX, newX);
                state.setInputParameter(inputIndex, WFSParameterIDs::inputPositionY, newY);

                updatedPositions.emplace_back(inputIndex + 1, newX, newY);
            }
        }
        else // Rotation
        {
            // Apply rotation around reference point
            float angleDeg = parsed.value;
            float angleRad = angleDeg * (juce::MathConstants<float>::pi / 180.0f);
            float cosA = std::cos(angleRad);
            float sinA = std::sin(angleRad);

            for (int inputIndex : clusterInputs)
            {
                juce::var posXVar = state.getInputParameter(inputIndex, WFSParameterIDs::inputPositionX);
                juce::var posYVar = state.getInputParameter(inputIndex, WFSParameterIDs::inputPositionY);

                float x = varToFloat(posXVar);
                float y = varToFloat(posYVar);

                // Rotate around reference point (XY plane)
                float dx = x - refX;
                float dy = y - refY;
                float newX = refX + dx * cosA - dy * sinA;
                float newY = refY + dx * sinA + dy * cosA;

                state.setInputParameter(inputIndex, WFSParameterIDs::inputPositionX, newX);
                state.setInputParameter(inputIndex, WFSParameterIDs::inputPositionY, newY);

                updatedPositions.emplace_back(inputIndex + 1, newX, newY);
            }
        }

        incomingProtocol = Protocol::Disabled;  // Clear flag

        // Echo updated positions back to Remote targets so Android sees all members move
        for (const auto& [channelId, x, y] : updatedPositions)
            sendInputPositionXYToRemote(channelId, x, y);
    });
}

void OSCManager::sendRemoteChannelDump(int channelId)
{
    // Convert 1-based channelId to 0-based index for internal API
    int channelIndex = channelId - 1;
    if (channelIndex < 0)
        return;

    // Helper lambda to get a parameter value as float (uses 0-based channelIndex)
    auto getParam = [this, channelIndex](const juce::Identifier& paramId) -> float {
        juce::var val = state.getInputParameter(channelIndex, paramId);
        if (val.isDouble())
            return static_cast<float>(static_cast<double>(val));
        if (val.isInt())
            return static_cast<float>(static_cast<int>(val));
        if (val.isBool())
            return static_cast<bool>(val) ? 1.0f : 0.0f;
        return 0.0f;
    };

    // Collect all input parameters for this channel
    std::map<juce::Identifier, float> paramValues;

    // Helper lambda to get an integer parameter value (uses 0-based channelIndex)
    auto getIntParam = [this, channelIndex](const juce::Identifier& paramId) -> int {
        juce::var val = state.getInputParameter(channelIndex, paramId);
        if (val.isInt())
            return static_cast<int>(val);
        if (val.isDouble())
            return static_cast<int>(static_cast<double>(val));
        if (val.isBool())
            return static_cast<bool>(val) ? 1 : 0;
        return 0;
    };

    // Integer parameters - sent with ,ii type tag
    std::map<juce::Identifier, int> intParamValues;
    intParamValues[WFSParameterIDs::inputCluster] = getIntParam(WFSParameterIDs::inputCluster);

    // Channel parameters
    paramValues[WFSParameterIDs::inputAttenuation] = getParam(WFSParameterIDs::inputAttenuation);
    paramValues[WFSParameterIDs::inputDelayLatency] = getParam(WFSParameterIDs::inputDelayLatency);
    paramValues[WFSParameterIDs::inputMinimalLatency] = getParam(WFSParameterIDs::inputMinimalLatency);

    // Position parameters
    paramValues[WFSParameterIDs::inputPositionX] = getParam(WFSParameterIDs::inputPositionX);
    paramValues[WFSParameterIDs::inputPositionY] = getParam(WFSParameterIDs::inputPositionY);
    paramValues[WFSParameterIDs::inputPositionZ] = getParam(WFSParameterIDs::inputPositionZ);
    paramValues[WFSParameterIDs::inputOffsetX] = getParam(WFSParameterIDs::inputOffsetX);
    paramValues[WFSParameterIDs::inputOffsetY] = getParam(WFSParameterIDs::inputOffsetY);
    paramValues[WFSParameterIDs::inputOffsetZ] = getParam(WFSParameterIDs::inputOffsetZ);
    paramValues[WFSParameterIDs::inputMaxSpeedActive] = getParam(WFSParameterIDs::inputMaxSpeedActive);
    paramValues[WFSParameterIDs::inputMaxSpeed] = getParam(WFSParameterIDs::inputMaxSpeed);
    paramValues[WFSParameterIDs::inputPathModeActive] = getParam(WFSParameterIDs::inputPathModeActive);
    paramValues[WFSParameterIDs::inputHeightFactor] = getParam(WFSParameterIDs::inputHeightFactor);
    paramValues[WFSParameterIDs::inputCoordinateMode] = getParam(WFSParameterIDs::inputCoordinateMode);
    paramValues[WFSParameterIDs::inputConstraintDistance] = getParam(WFSParameterIDs::inputConstraintDistance);
    paramValues[WFSParameterIDs::inputConstraintDistanceMin] = getParam(WFSParameterIDs::inputConstraintDistanceMin);
    paramValues[WFSParameterIDs::inputConstraintDistanceMax] = getParam(WFSParameterIDs::inputConstraintDistanceMax);

    // Attenuation parameters
    paramValues[WFSParameterIDs::inputAttenuationLaw] = getParam(WFSParameterIDs::inputAttenuationLaw);
    paramValues[WFSParameterIDs::inputDistanceAttenuation] = getParam(WFSParameterIDs::inputDistanceAttenuation);
    paramValues[WFSParameterIDs::inputDistanceRatio] = getParam(WFSParameterIDs::inputDistanceRatio);
    paramValues[WFSParameterIDs::inputCommonAtten] = getParam(WFSParameterIDs::inputCommonAtten);

    // Directivity parameters
    paramValues[WFSParameterIDs::inputDirectivity] = getParam(WFSParameterIDs::inputDirectivity);
    paramValues[WFSParameterIDs::inputRotation] = getParam(WFSParameterIDs::inputRotation);
    paramValues[WFSParameterIDs::inputTilt] = getParam(WFSParameterIDs::inputTilt);
    paramValues[WFSParameterIDs::inputHFshelf] = getParam(WFSParameterIDs::inputHFshelf);

    // Live Source Tamer parameters
    paramValues[WFSParameterIDs::inputLSactive] = getParam(WFSParameterIDs::inputLSactive);
    paramValues[WFSParameterIDs::inputLSradius] = getParam(WFSParameterIDs::inputLSradius);
    paramValues[WFSParameterIDs::inputLSshape] = getParam(WFSParameterIDs::inputLSshape);
    paramValues[WFSParameterIDs::inputLSattenuation] = getParam(WFSParameterIDs::inputLSattenuation);
    paramValues[WFSParameterIDs::inputLSpeakThreshold] = getParam(WFSParameterIDs::inputLSpeakThreshold);
    paramValues[WFSParameterIDs::inputLSpeakRatio] = getParam(WFSParameterIDs::inputLSpeakRatio);
    paramValues[WFSParameterIDs::inputLSslowThreshold] = getParam(WFSParameterIDs::inputLSslowThreshold);
    paramValues[WFSParameterIDs::inputLSslowRatio] = getParam(WFSParameterIDs::inputLSslowRatio);

    // Hackoustics (Floor Reflections) parameters
    paramValues[WFSParameterIDs::inputFRactive] = getParam(WFSParameterIDs::inputFRactive);
    paramValues[WFSParameterIDs::inputFRattenuation] = getParam(WFSParameterIDs::inputFRattenuation);
    paramValues[WFSParameterIDs::inputFRlowCutActive] = getParam(WFSParameterIDs::inputFRlowCutActive);
    paramValues[WFSParameterIDs::inputFRlowCutFreq] = getParam(WFSParameterIDs::inputFRlowCutFreq);
    paramValues[WFSParameterIDs::inputFRhighShelfActive] = getParam(WFSParameterIDs::inputFRhighShelfActive);
    paramValues[WFSParameterIDs::inputFRhighShelfFreq] = getParam(WFSParameterIDs::inputFRhighShelfFreq);
    paramValues[WFSParameterIDs::inputFRhighShelfGain] = getParam(WFSParameterIDs::inputFRhighShelfGain);
    paramValues[WFSParameterIDs::inputFRhighShelfSlope] = getParam(WFSParameterIDs::inputFRhighShelfSlope);
    paramValues[WFSParameterIDs::inputFRdiffusion] = getParam(WFSParameterIDs::inputFRdiffusion);

    // Jitter
    paramValues[WFSParameterIDs::inputJitter] = getParam(WFSParameterIDs::inputJitter);

    // LFO parameters
    paramValues[WFSParameterIDs::inputLFOactive] = getParam(WFSParameterIDs::inputLFOactive);
    paramValues[WFSParameterIDs::inputLFOperiod] = getParam(WFSParameterIDs::inputLFOperiod);
    paramValues[WFSParameterIDs::inputLFOphase] = getParam(WFSParameterIDs::inputLFOphase);
    paramValues[WFSParameterIDs::inputLFOshapeX] = getParam(WFSParameterIDs::inputLFOshapeX);
    paramValues[WFSParameterIDs::inputLFOshapeY] = getParam(WFSParameterIDs::inputLFOshapeY);
    paramValues[WFSParameterIDs::inputLFOshapeZ] = getParam(WFSParameterIDs::inputLFOshapeZ);
    paramValues[WFSParameterIDs::inputLFOrateX] = getParam(WFSParameterIDs::inputLFOrateX);
    paramValues[WFSParameterIDs::inputLFOrateY] = getParam(WFSParameterIDs::inputLFOrateY);
    paramValues[WFSParameterIDs::inputLFOrateZ] = getParam(WFSParameterIDs::inputLFOrateZ);
    paramValues[WFSParameterIDs::inputLFOamplitudeX] = getParam(WFSParameterIDs::inputLFOamplitudeX);
    paramValues[WFSParameterIDs::inputLFOamplitudeY] = getParam(WFSParameterIDs::inputLFOamplitudeY);
    paramValues[WFSParameterIDs::inputLFOamplitudeZ] = getParam(WFSParameterIDs::inputLFOamplitudeZ);
    paramValues[WFSParameterIDs::inputLFOphaseX] = getParam(WFSParameterIDs::inputLFOphaseX);
    paramValues[WFSParameterIDs::inputLFOphaseY] = getParam(WFSParameterIDs::inputLFOphaseY);
    paramValues[WFSParameterIDs::inputLFOphaseZ] = getParam(WFSParameterIDs::inputLFOphaseZ);
    paramValues[WFSParameterIDs::inputLFOgyrophone] = getParam(WFSParameterIDs::inputLFOgyrophone);

    // Tracking (read-only on Remote side)
    paramValues[WFSParameterIDs::inputTrackingActive] = getParam(WFSParameterIDs::inputTrackingActive);

    // Build and send messages (float params as ,if, int params as ,ii)
    auto messages = OSCMessageBuilder::buildRemoteChannelDump(channelId, paramValues, intParamValues);

    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        const auto& config = targetConfigs[static_cast<size_t>(i)];
        if (config.protocol == Protocol::Remote && config.txEnabled)
        {
            for (const auto& msg : messages)
            {
                sendMessage(i, msg);
            }
        }
    }

    // Send input name (string parameter - needs separate handling)
    juce::var nameVal = state.getInputParameter(channelIndex, WFSParameterIDs::inputName);
    if (nameVal.isString())
    {
        auto nameMsg = OSCMessageBuilder::buildRemoteOutputStringMessage(
            WFSParameterIDs::inputName, channelId, nameVal.toString());

        if (nameMsg.has_value())
        {
            for (int i = 0; i < MAX_TARGETS; ++i)
            {
                const auto& config = targetConfigs[static_cast<size_t>(i)];
                if (config.protocol == Protocol::Remote && config.txEnabled)
                {
                    sendMessage(i, *nameMsg);
                }
            }
        }
    }

    // Also send the computed "fully tracked" state (depends on global settings too)
    sendInputFullyTrackedState(channelId);
}

bool OSCManager::isInputFullyTracked(int channelIndex) const
{
    // Get global tracking settings
    bool globalTracking = state.getIntParameter(WFSParameterIDs::trackingEnabled) != 0;
    bool protocolEnabled = state.getIntParameter(WFSParameterIDs::trackingProtocol) != 0;

    // Get per-input tracking setting
    bool localTracking = state.getIntParameter(WFSParameterIDs::inputTrackingActive, channelIndex) != 0;

    return globalTracking && protocolEnabled && localTracking;
}

void OSCManager::sendInputFullyTrackedState(int channelId)
{
    int channelIndex = channelId - 1;
    if (channelIndex < 0)
        return;

    bool fullyTracked = isInputFullyTracked(channelIndex);

    // Build OSC message: /remoteInput/isFullyTracked <channelId> <0 or 1>
    juce::OSCMessage msg("/remoteInput/isFullyTracked");
    msg.addInt32(channelId);
    msg.addInt32(fullyTracked ? 1 : 0);

    // Send to all connected Remote protocol targets
    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        const auto& config = targetConfigs[static_cast<size_t>(i)];
        if (config.protocol == Protocol::Remote && config.txEnabled &&
            remoteStates[static_cast<size_t>(i)].phase == RemoteConnectionState::Phase::Connected)
        {
            sendMessage(i, msg);
        }
    }
}

void OSCManager::sendAllTrackingStatesToRemote()
{
    int numInputs = state.getIntParameter(WFSParameterIDs::inputChannels);

    // Collect all messages first, then send directly to bypass rate limiter.
    // The rate limiter coalesces by OSC address, so bulk sends for all channels
    // would result in only the last channel's data being sent.
    std::vector<juce::OSCMessage> messages;
    messages.reserve(static_cast<size_t>(numInputs));

    for (int i = 0; i < numInputs; ++i)
    {
        int channelId = i + 1;  // 1-based
        bool fullyTracked = isInputFullyTracked(i);

        juce::OSCMessage msg("/remoteInput/isFullyTracked");
        msg.addInt32(channelId);
        msg.addInt32(fullyTracked ? 1 : 0);
        messages.push_back(std::move(msg));
    }

    // Send directly to all connected Remote targets
    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        const auto& config = targetConfigs[static_cast<size_t>(i)];
        if (config.protocol == Protocol::Remote && config.txEnabled &&
            remoteStates[static_cast<size_t>(i)].phase == RemoteConnectionState::Phase::Connected)
        {
            if (connections[static_cast<size_t>(i)])
            {
                for (const auto& msg : messages)
                {
                    if (connections[static_cast<size_t>(i)]->send(msg))
                    {
                        ++messagesSent;
                        logger.logSentWithDetails(i, msg, config.protocol,
                                                  config.ipAddress, config.port, config.mode);
                    }
                }
            }
        }
    }
}

void OSCManager::sendAllClusterConfigsToRemote(int targetIndex)
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    if (targetIndex < 0 || targetIndex >= MAX_TARGETS)
        return;

    const auto& config = targetConfigs[static_cast<size_t>(targetIndex)];
    if (!connections[static_cast<size_t>(targetIndex)])
        return;

    auto sendDirect = [this, targetIndex, &config](const juce::OSCMessage& msg)
    {
        if (connections[static_cast<size_t>(targetIndex)]->send(msg))
        {
            ++messagesSent;
            logger.logSentWithDetails(targetIndex, msg, config.protocol,
                                      config.ipAddress, config.port, config.mode);
        }
    };

    int numInputs = state.getIntParameter(inputChannels);

    for (int c = 1; c <= maxClusters; ++c)
    {
        // Send reference mode (getClusterParameter takes 1-based cluster ID)
        juce::var refModeVar = state.getClusterParameter(c, clusterReferenceMode);
        int refMode = refModeVar.isVoid() ? 0 : static_cast<int>(refModeVar);

        juce::OSCMessage msgRefMode("/cluster/referenceMode");
        msgRefMode.addInt32(c);
        msgRefMode.addInt32(refMode);
        sendDirect(msgRefMode);

        // Find tracked input for this cluster (first fully-tracked input in the cluster)
        int trackedInputId = 0;
        for (int i = 0; i < numInputs; ++i)
        {
            juce::var clusterVar = state.getInputParameter(i, inputCluster);
            int inputClusterIdx = clusterVar.isVoid() ? 0 : static_cast<int>(clusterVar);

            if (inputClusterIdx == c && isInputFullyTracked(i))
            {
                trackedInputId = i + 1;  // 1-based
                break;
            }
        }

        juce::OSCMessage msgTracked("/cluster/trackedInput");
        msgTracked.addInt32(c);
        msgTracked.addInt32(trackedInputId);
        sendDirect(msgTracked);
    }

    DBG("OSCManager: Sent cluster configs for " << maxClusters << " clusters to target " << targetIndex);
}

void OSCManager::sendStageConfigToRemote()
{
    auto stageTree = state.getStageState();
    if (!stageTree.isValid())
        return;

    // Gather stage parameters
    float originX = static_cast<float>(stageTree.getProperty(WFSParameterIDs::originWidth));
    float originY = static_cast<float>(stageTree.getProperty(WFSParameterIDs::originDepth));
    float originZ = static_cast<float>(stageTree.getProperty(WFSParameterIDs::originHeight));
    float width = static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageWidth));
    float depth = static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageDepth));
    float height = static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageHeight));
    int shape = static_cast<int>(stageTree.getProperty(WFSParameterIDs::stageShape));
    float diameter = static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageDiameter));
    float domeElev = static_cast<float>(stageTree.getProperty(WFSParameterIDs::domeElevation));

    // Build messages (note: /inputs is sent separately in onRemoteConnected before positions)
    std::vector<juce::OSCMessage> messages;
    messages.push_back(OSCMessageBuilder::buildConfigFloatMessage("/stage/originX", originX));
    messages.push_back(OSCMessageBuilder::buildConfigFloatMessage("/stage/originY", originY));
    messages.push_back(OSCMessageBuilder::buildConfigFloatMessage("/stage/originZ", originZ));
    messages.push_back(OSCMessageBuilder::buildConfigFloatMessage("/stage/width", width));
    messages.push_back(OSCMessageBuilder::buildConfigFloatMessage("/stage/depth", depth));
    messages.push_back(OSCMessageBuilder::buildConfigFloatMessage("/stage/height", height));
    messages.push_back(OSCMessageBuilder::buildConfigIntMessage("/stage/shape", shape));
    messages.push_back(OSCMessageBuilder::buildConfigFloatMessage("/stage/diameter", diameter));
    messages.push_back(OSCMessageBuilder::buildConfigFloatMessage("/stage/domeElevation", domeElev));
    // Note: /inputs is sent separately in onRemoteConnected() before positions

    // Send to all Remote protocol targets that are connected
    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        if (targetConfigs[static_cast<size_t>(i)].protocol == Protocol::Remote &&
            targetStatuses[static_cast<size_t>(i)] == ConnectionStatus::Connected)
        {
            for (const auto& msg : messages)
                sendMessage(i, msg);
        }
    }
}

void OSCManager::sendFindDevice(const juce::String& password)
{
    // Build /findDevice message with password argument
    juce::OSCMessage msg("/findDevice");
    msg.addString(password);

    // Send to all connected REMOTE targets
    int targetsSent = 0;
    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        if (targetConfigs[static_cast<size_t>(i)].protocol == Protocol::Remote &&
            targetStatuses[static_cast<size_t>(i)] == ConnectionStatus::Connected)
        {
            sendMessage(i, msg);
            ++targetsSent;
        }
    }

    DBG("OSCManager::sendFindDevice sent to " << targetsSent << " REMOTE target(s)");
}

void OSCManager::sendCompositeDeltaToRemote(int inputId, float deltaX, float deltaY)
{
    // Build /remoteInput/compositeDelta message: inputId (int), deltaX (float), deltaY (float)
    // Delta is the difference between composite position and target position
    juce::OSCMessage msg("/remoteInput/compositeDelta");
    msg.addInt32(inputId);
    msg.addFloat32(deltaX);
    msg.addFloat32(deltaY);

    // Send to all connected REMOTE targets
    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        if (targetConfigs[static_cast<size_t>(i)].protocol == Protocol::Remote &&
            targetStatuses[static_cast<size_t>(i)] == ConnectionStatus::Connected)
        {
            sendMessage(i, msg);
        }
    }
}

void OSCManager::updateTargetStatus(int targetIndex, ConnectionStatus newStatus)
{
    if (targetIndex < 0 || targetIndex >= MAX_TARGETS)
        return;

    targetStatuses[static_cast<size_t>(targetIndex)] = newStatus;

    // Note: For Remote protocol, stage config is sent via onRemoteConnected() after handshake

    if (onConnectionStatusChanged)
        onConnectionStatusChanged(targetIndex, newStatus);
}

//==============================================================================
// Stage Bounds
//==============================================================================

float OSCManager::getStageMinX() const
{
    auto stageTree = state.getStageState();
    if (!stageTree.isValid())
        return -10.0f;

    int shape = stageTree.getProperty(WFSParameterIDs::stageShape);
    float halfSize = (shape == 0)
        ? static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageWidth)) / 2.0f
        : static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageDiameter)) / 2.0f;
    float originWidth = static_cast<float>(stageTree.getProperty(WFSParameterIDs::originWidth));
    return -halfSize - originWidth;
}

float OSCManager::getStageMaxX() const
{
    auto stageTree = state.getStageState();
    if (!stageTree.isValid())
        return 10.0f;

    int shape = stageTree.getProperty(WFSParameterIDs::stageShape);
    float halfSize = (shape == 0)
        ? static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageWidth)) / 2.0f
        : static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageDiameter)) / 2.0f;
    float originWidth = static_cast<float>(stageTree.getProperty(WFSParameterIDs::originWidth));
    return halfSize - originWidth;
}

float OSCManager::getStageMinY() const
{
    auto stageTree = state.getStageState();
    if (!stageTree.isValid())
        return -5.0f;

    int shape = stageTree.getProperty(WFSParameterIDs::stageShape);
    float halfSize = (shape == 0)
        ? static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageDepth)) / 2.0f
        : static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageDiameter)) / 2.0f;
    float originDepth = static_cast<float>(stageTree.getProperty(WFSParameterIDs::originDepth));
    return -halfSize - originDepth;
}

float OSCManager::getStageMaxY() const
{
    auto stageTree = state.getStageState();
    if (!stageTree.isValid())
        return 5.0f;

    int shape = stageTree.getProperty(WFSParameterIDs::stageShape);
    float halfSize = (shape == 0)
        ? static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageDepth)) / 2.0f
        : static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageDiameter)) / 2.0f;
    float originDepth = static_cast<float>(stageTree.getProperty(WFSParameterIDs::originDepth));
    return halfSize - originDepth;
}

float OSCManager::getStageMaxZ() const
{
    auto stageTree = state.getStageState();
    if (!stageTree.isValid())
        return 5.0f;

    return static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageHeight));
}

//==============================================================================
// Constraint Application
//==============================================================================

float OSCManager::applyConstraintX(int channelIndex, float value) const
{
    juce::var constraintVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputConstraintX);
    int constraint = constraintVar.isInt() ? static_cast<int>(constraintVar) : 1;

    if (constraint != 0)
    {
        // Check if distance constraint should take precedence over rectangular X bound
        juce::var coordModeVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputCoordinateMode);
        int coordMode = coordModeVar.isInt() ? static_cast<int>(coordModeVar) : 0;
        juce::var constraintDistVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputConstraintDistance);
        int constraintDist = constraintDistVar.isInt() ? static_cast<int>(constraintDistVar) : 0;

        // Skip rectangular X bound in cylindrical/spherical mode when distance constraint is enabled
        if ((coordMode == 1 || coordMode == 2) && constraintDist != 0)
            return value;

        return juce::jlimit(getStageMinX(), getStageMaxX(), value);
    }

    return value;
}

float OSCManager::applyConstraintY(int channelIndex, float value) const
{
    juce::var constraintVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputConstraintY);
    int constraint = constraintVar.isInt() ? static_cast<int>(constraintVar) : 1;

    if (constraint != 0)
    {
        // Check if distance constraint should take precedence over rectangular Y bound
        juce::var coordModeVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputCoordinateMode);
        int coordMode = coordModeVar.isInt() ? static_cast<int>(coordModeVar) : 0;
        juce::var constraintDistVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputConstraintDistance);
        int constraintDist = constraintDistVar.isInt() ? static_cast<int>(constraintDistVar) : 0;

        // Skip rectangular Y bound in cylindrical/spherical mode when distance constraint is enabled
        if ((coordMode == 1 || coordMode == 2) && constraintDist != 0)
            return value;

        return juce::jlimit(getStageMinY(), getStageMaxY(), value);
    }

    return value;
}

float OSCManager::applyConstraintZ(int channelIndex, float value) const
{
    juce::var constraintVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputConstraintZ);
    int constraint = constraintVar.isInt() ? static_cast<int>(constraintVar) : 1;

    if (constraint != 0)
    {
        // Check if distance constraint should take precedence over rectangular Z bound
        juce::var coordModeVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputCoordinateMode);
        int coordMode = coordModeVar.isInt() ? static_cast<int>(coordModeVar) : 0;
        juce::var constraintDistVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputConstraintDistance);
        int constraintDist = constraintDistVar.isInt() ? static_cast<int>(constraintDistVar) : 0;

        // Skip rectangular Z bound in spherical mode when distance constraint is enabled
        if (coordMode == 2 && constraintDist != 0)
            return value;

        return juce::jlimit(0.0f, getStageMaxZ(), value);
    }

    return value;
}

void OSCManager::applyConstraintDistance(int channelIndex, float& x, float& y, float& z) const
{
    // Get coordinate mode
    juce::var coordModeVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputCoordinateMode);
    int coordMode = coordModeVar.isInt() ? static_cast<int>(coordModeVar) : 0;

    // Only apply in Cylindrical (1) or Spherical (2) modes
    if (coordMode != 1 && coordMode != 2)
        return;

    // Check if distance constraint is enabled
    juce::var constraintDistVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputConstraintDistance);
    int constraintDist = constraintDistVar.isInt() ? static_cast<int>(constraintDistVar) : 0;
    if (constraintDist == 0)
        return;

    // Get min/max distance values
    juce::var minDistVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputConstraintDistanceMin);
    juce::var maxDistVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputConstraintDistanceMax);
    float minDist = varToFloat(minDistVar, 0.0f);
    float maxDist = varToFloat(maxDistVar, 50.0f);

    // Calculate current distance
    float currentDist;
    if (coordMode == 1)  // Cylindrical: radial distance in XY plane
        currentDist = std::sqrt(x * x + y * y);
    else  // Spherical: full 3D distance
        currentDist = std::sqrt(x * x + y * y + z * z);

    // Avoid division by zero
    if (currentDist < 0.0001f)
        currentDist = 0.0001f;

    float targetDist = juce::jlimit(minDist, maxDist, currentDist);

    if (!juce::approximatelyEqual(currentDist, targetDist))
    {
        float scale = targetDist / currentDist;
        if (coordMode == 1)  // Cylindrical: scale X, Y only
        {
            x *= scale;
            y *= scale;
        }
        else  // Spherical: scale X, Y, Z
        {
            x *= scale;
            y *= scale;
            z *= scale;
        }
    }
}

//==============================================================================
// Remote Handshake/Heartbeat Methods
//==============================================================================

void OSCManager::sendRemotePing(int targetIndex)
{
    if (targetIndex < 0 || targetIndex >= MAX_TARGETS)
        return;

    auto& remoteState = remoteStates[static_cast<size_t>(targetIndex)];
    int seqNum = remoteState.nextSequenceNumber++;
    remoteState.pendingSequenceNumber = seqNum;

    juce::OSCMessage msg("/remote/ping");
    msg.addInt32(seqNum);
    sendMessage(targetIndex, msg);

    DBG("OSCManager: Sent /remote/ping seq=" << seqNum << " to target " << targetIndex);
}

void OSCManager::sendRemoteHeartbeat(int targetIndex)
{
    if (targetIndex < 0 || targetIndex >= MAX_TARGETS)
        return;

    auto& remoteState = remoteStates[static_cast<size_t>(targetIndex)];
    int seqNum = remoteState.nextSequenceNumber++;
    remoteState.pendingSequenceNumber = seqNum;

    juce::OSCMessage msg("/remote/heartbeat");
    msg.addInt32(seqNum);
    sendMessage(targetIndex, msg);
}

void OSCManager::handleRemotePong(int targetIndex, int sequenceNumber)
{
    if (targetIndex < 0 || targetIndex >= MAX_TARGETS)
        return;

    auto& remoteState = remoteStates[static_cast<size_t>(targetIndex)];

    // Validate sequence number
    if (sequenceNumber != remoteState.pendingSequenceNumber)
    {
        DBG("OSCManager: Received /remote/pong with unexpected seq=" << sequenceNumber
            << " (expected " << remoteState.pendingSequenceNumber << ")");
        return;
    }

    remoteState.lastPongReceivedTime = juce::Time::currentTimeMillis();

    if (remoteState.phase == RemoteConnectionState::Phase::Connecting)
    {
        bool isReconnection = remoteState.wasConnectedBefore;
        remoteState.phase = RemoteConnectionState::Phase::Connected;
        remoteState.wasConnectedBefore = true;

        // Update target status to trigger onConnectionStatusChanged callback
        updateTargetStatus(targetIndex, ConnectionStatus::Connected);

        onRemoteConnected(targetIndex, isReconnection);
    }

    DBG("OSCManager: Received /remote/pong seq=" << sequenceNumber << " from target " << targetIndex);
}

void OSCManager::handleRemoteHeartbeatAck(int targetIndex, int sequenceNumber)
{
    if (targetIndex < 0 || targetIndex >= MAX_TARGETS)
        return;

    auto& remoteState = remoteStates[static_cast<size_t>(targetIndex)];

    // Validate sequence number
    if (sequenceNumber != remoteState.pendingSequenceNumber)
    {
        DBG("OSCManager: Received /remote/heartbeatAck with unexpected seq=" << sequenceNumber);
        return;
    }

    remoteState.lastPongReceivedTime = juce::Time::currentTimeMillis();
}

void OSCManager::onRemoteConnected(int targetIndex, bool isReconnection)
{
    DBG("OSCManager: Remote target " << targetIndex << " connected"
        << (isReconnection ? " (reconnection)" : " (initial)"));

    // Delay the initial state dump to let the connection stabilize
    // Use callAfterDelay to send data after 500ms
    juce::Timer::callAfterDelay(500, [this, targetIndex]()
    {
        // Verify the target is still connected before sending
        auto& remoteState = remoteStates[static_cast<size_t>(targetIndex)];
        if (remoteState.phase != RemoteConnectionState::Phase::Connected)
            return;

        DBG("OSCManager: Sending initial state dump to target " << targetIndex);

        // Send /inputs FIRST so Android expands its data structure before receiving positions
        auto ioTree = state.getIOState();
        int inputs = ioTree.isValid()
            ? static_cast<int>(ioTree.getProperty(WFSParameterIDs::inputChannels))
            : 8;
        juce::OSCMessage inputsMsg = OSCMessageBuilder::buildConfigIntMessage("/inputs", inputs);
        sendMessage(targetIndex, inputsMsg);

        // Send stage configuration (dimensions, shape, etc.)
        sendStageConfigToRemote();

        // Send cluster configs BEFORE input positions so Android knows
        // the reference mode before receiving cluster assignments
        sendAllClusterConfigsToRemote(targetIndex);

        // Send all input positions (includes cluster assignments)
        sendAllInputPositionsToRemote(targetIndex);

        // Send tracking state for all inputs
        sendAllTrackingStatesToRemote();

        // Notify that connection is ready and initial data has been sent
        // This allows MainComponent to send composite deltas
        if (onRemoteConnectionReady)
            onRemoteConnectionReady(targetIndex);
    });
}

void OSCManager::onRemoteDisconnected(int targetIndex)
{
    if (targetIndex < 0 || targetIndex >= MAX_TARGETS)
        return;

    auto& remoteState = remoteStates[static_cast<size_t>(targetIndex)];
    remoteState.phase = RemoteConnectionState::Phase::Disconnected;

    DBG("OSCManager: Remote target " << targetIndex << " disconnected");

    // Optionally send disconnect message (best effort, may not arrive)
    juce::OSCMessage msg("/remote/disconnect");
    sendMessage(targetIndex, msg);
}

void OSCManager::resendStateToRemoteTargets()
{
    // Send /inputs first, then stage config, then positions to all connected Remote targets
    auto ioTree = state.getIOState();
    int inputs = ioTree.isValid()
        ? static_cast<int>(ioTree.getProperty(WFSParameterIDs::inputChannels))
        : 8;
    juce::OSCMessage inputsMsg = OSCMessageBuilder::buildConfigIntMessage("/inputs", inputs);

    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        if (targetConfigs[static_cast<size_t>(i)].protocol == Protocol::Remote &&
            remoteStates[static_cast<size_t>(i)].phase == RemoteConnectionState::Phase::Connected)
        {
            sendMessage(i, inputsMsg);
            // Send cluster configs BEFORE positions so Android knows reference modes
            sendAllClusterConfigsToRemote(i);
            sendAllInputPositionsToRemote(i);
        }
    }

    sendStageConfigToRemote();
    sendAllTrackingStatesToRemote();
}

void OSCManager::sendAllInputPositionsToRemote(int targetIndex)
{
    if (targetIndex < 0 || targetIndex >= MAX_TARGETS)
        return;

    // Verify target is connected
    const auto& config = targetConfigs[static_cast<size_t>(targetIndex)];
    if (!connections[static_cast<size_t>(targetIndex)])
        return;

    // Helper lambda to send directly, bypassing rate limiter
    // This is critical for bulk sends where multiple channels share the same OSC address.
    // The rate limiter coalesces by address, so only the last channel would be sent otherwise.
    auto sendDirect = [this, targetIndex, &config](const juce::OSCMessage& msg)
    {
        if (connections[static_cast<size_t>(targetIndex)]->send(msg))
        {
            ++messagesSent;
            logger.logSentWithDetails(targetIndex, msg, config.protocol,
                                      config.ipAddress, config.port, config.mode);
        }
    };

    // Get number of input channels
    auto ioTree = state.getIOState();
    int numInputs = ioTree.isValid()
        ? static_cast<int>(ioTree.getProperty(WFSParameterIDs::inputChannels))
        : 8;

    // Send data for each input channel
    for (int channelIndex = 0; channelIndex < numInputs; ++channelIndex)
    {
        int channelId = channelIndex + 1;  // 1-based for OSC messages

        // Get and send input name
        juce::var nameVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputName);
        if (nameVar.isString())
        {
            juce::OSCMessage msgName("/remoteInput/inputName");
            msgName.addInt32(channelId);
            msgName.addString(nameVar.toString());
            sendDirect(msgName);
        }

        // Get position values
        juce::var posXVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionX);
        juce::var posYVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionY);
        juce::var posZVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionZ);

        // Use var's built-in conversion which handles both int and double types
        // (XML loading may store integer values like "3" as int, not double)
        float posX = varToFloat(posXVar);
        float posY = varToFloat(posYVar);
        float posZ = varToFloat(posZVar);

        // Build and send position messages
        juce::OSCMessage msgX("/remoteInput/positionX");
        msgX.addInt32(channelId);
        msgX.addFloat32(posX);
        sendDirect(msgX);

        juce::OSCMessage msgY("/remoteInput/positionY");
        msgY.addInt32(channelId);
        msgY.addFloat32(posY);
        sendDirect(msgY);

        juce::OSCMessage msgZ("/remoteInput/positionZ");
        msgZ.addInt32(channelId);
        msgZ.addFloat32(posZ);
        sendDirect(msgZ);

        // Get and send offset values
        juce::var offXVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputOffsetX);
        juce::var offYVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputOffsetY);
        juce::var offZVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputOffsetZ);

        float offX = varToFloat(offXVar);
        float offY = varToFloat(offYVar);
        float offZ = varToFloat(offZVar);

        juce::OSCMessage msgOffX("/remoteInput/offsetX");
        msgOffX.addInt32(channelId);
        msgOffX.addFloat32(offX);
        sendDirect(msgOffX);

        juce::OSCMessage msgOffY("/remoteInput/offsetY");
        msgOffY.addInt32(channelId);
        msgOffY.addFloat32(offY);
        sendDirect(msgOffY);

        juce::OSCMessage msgOffZ("/remoteInput/offsetZ");
        msgOffZ.addInt32(channelId);
        msgOffZ.addFloat32(offZ);
        sendDirect(msgOffZ);

        // Get and send cluster assignment as int
        juce::var clusterVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputCluster);
        int clusterValue = clusterVar.isVoid() ? 0 : static_cast<int>(clusterVar);
        juce::OSCMessage msgCluster("/remoteInput/cluster");
        msgCluster.addInt32(channelId);
        msgCluster.addInt32(clusterValue);
        sendDirect(msgCluster);
    }

    DBG("OSCManager: Sent names, positions, offsets and clusters for " << numInputs << " inputs to target " << targetIndex);
}

int OSCManager::findRemoteTargetByIP(const juce::String& senderIP) const
{
    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        const auto& config = targetConfigs[static_cast<size_t>(i)];
        if (config.protocol == Protocol::Remote && config.txEnabled)
        {
            // Match against the target's configured IP
            if (config.ipAddress == senderIP)
                return i;
        }
    }
    return -1;
}

void OSCManager::sendInputPositionXYToRemote(int channelId, float x, float y)
{
    // Build combined XY message: /remoteInput/positionXY <channelId:i> <x:f> <y:f>
    juce::OSCMessage msg("/remoteInput/positionXY");
    msg.addInt32(channelId);
    msg.addFloat32(x);
    msg.addFloat32(y);

    // Send directly to all connected Remote targets, bypassing rate limiter
    // This ensures immediate position updates for smooth movement.
    // Since we're already batching X+Y into a single message, the message rate
    // is already reduced by half, making rate limiting unnecessary here.
    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        const auto& config = targetConfigs[static_cast<size_t>(i)];
        if (config.protocol == Protocol::Remote &&
            config.txEnabled &&
            remoteStates[static_cast<size_t>(i)].phase == RemoteConnectionState::Phase::Connected)
        {
            // Skip if this message originated from a Remote target (loop prevention)
            if (incomingProtocol == Protocol::Remote)
                continue;

            // Send directly through connection, bypassing rate limiter for zero latency
            if (connections[static_cast<size_t>(i)])
            {
                if (connections[static_cast<size_t>(i)]->send(msg))
                {
                    ++messagesSent;
                    logger.logSentWithDetails(i, msg, config.protocol,
                                              config.ipAddress, config.port, config.mode);
                }
            }
        }
    }
}

} // namespace WFSNetwork
