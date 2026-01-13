#include "OSCManager.h"

namespace WFSNetwork
{

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
        DBG("OSCManager rate limiter callback - target " << targetIndex
            << " addr=" << message.getAddressPattern().toString());

        if (targetIndex >= 0 && targetIndex < MAX_TARGETS)
        {
            if (connections[static_cast<size_t>(targetIndex)])
            {
                DBG("OSCManager rate limiter - sending to connection " << targetIndex);
                if (connections[static_cast<size_t>(targetIndex)]->send(message))
                {
                    ++messagesSent;
                    const auto& config = targetConfigs[static_cast<size_t>(targetIndex)];
                    logger.logSentWithDetails(targetIndex, message, config.protocol,
                                              config.ipAddress, config.port, config.mode);
                    DBG("OSCManager rate limiter - message sent successfully");
                }
                else
                {
                    DBG("OSCManager rate limiter - send FAILED");
                }
            }
            else
            {
                DBG("OSCManager rate limiter - no connection for target " << targetIndex);
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

    if (conn->connect())
    {
        updateTargetStatus(targetIndex, ConnectionStatus::Connected);
        logger.logText("Connected to target " + juce::String(targetIndex + 1) +
                       " (" + config.ipAddress + ":" + juce::String(config.port) + ")");
        DBG("OSCManager::connectTarget - target " << targetIndex << " CONNECTED");
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
    {
        DBG("OSCManager::sendMessage - target " << targetIndex << " disabled or txEnabled=false");
        return;
    }

    DBG("OSCManager::sendMessage - queuing message to target " << targetIndex
        << " addr=" << message.getAddressPattern().toString());
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
    // Debug: log all property changes
    DBG("OSCManager::valueTreePropertyChanged - tree=" << tree.getType().toString()
        << " property=" << property.toString()
        << " incomingProtocol=" << static_cast<int>(incomingProtocol));

    // Check if this is a stage/config parameter that needs broadcasting to Remote
    if (property == WFSParameterIDs::stageWidth ||
        property == WFSParameterIDs::stageDepth ||
        property == WFSParameterIDs::stageHeight ||
        property == WFSParameterIDs::stageDiameter ||
        property == WFSParameterIDs::stageShape ||
        property == WFSParameterIDs::originWidth ||
        property == WFSParameterIDs::originDepth ||
        property == WFSParameterIDs::originHeight ||
        property == WFSParameterIDs::inputChannels)
    {
        sendStageConfigToRemote();
        return;  // Don't process as channel parameter
    }

    // Determine if this is an input or output parameter change
    juce::String typeName = tree.getType().toString();
    juce::var value = tree.getProperty(property);

    // Find channel index by traversing up to Input or Output parent
    int channelId = -1;
    bool isInput = false;
    bool isOutput = false;

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
        {
            DBG("OSCManager: Skipping target " << i << " (same protocol as incoming)");
            continue;
        }

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
                DBG("OSCManager: Input param " << property.toString() << " ch" << channelId
                    << " value=" << static_cast<float>(static_cast<double>(value))
                    << " mapped=" << (msg.has_value() ? "yes" : "no"));
            }
            else if (isOutput && isNumeric)
            {
                msg = OSCMessageBuilder::buildOutputMessage(property, channelId,
                                                            static_cast<float>(static_cast<double>(value)));
                DBG("OSCManager: Output param " << property.toString() << " ch" << channelId
                    << " value=" << static_cast<float>(static_cast<double>(value))
                    << " mapped=" << (msg.has_value() ? "yes" : "no"));
            }

            if (msg.has_value())
            {
                DBG("OSCManager: Sending to target " << i << ": " << msg->getAddressPattern().toString());
                sendMessage(i, *msg);
            }
        }
        else if (config.protocol == Protocol::Remote)
        {
            // REMOTE protocol - only send for selected channel at 50Hz max
            if (isInput && channelId == remoteSelectedChannel)
            {
                bool isNumeric = value.isDouble() || value.isInt() || value.isInt64();
                if (isNumeric)
                {
                    auto msg = OSCMessageBuilder::buildRemoteOutputMessage(
                        property, channelId, static_cast<float>(static_cast<double>(value)));

                    if (msg.has_value())
                    {
                        sendMessage(i, *msg);
                    }
                }
            }
        }
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
    // Poll connection statuses
    for (int i = 0; i < MAX_TARGETS; ++i)
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
    if (OSCMessageRouter::isInputAddress(address) || OSCMessageRouter::isOutputAddress(address))
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

            if (OSCMessageRouter::isInputAddress(address) || OSCMessageRouter::isOutputAddress(address))
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
                        float x = xVar.isDouble() ? static_cast<float>(static_cast<double>(xVar)) : 0.0f;
                        float y = yVar.isDouble() ? static_cast<float>(static_cast<double>(yVar)) : 0.0f;
                        float z = zVar.isDouble() ? static_cast<float>(static_cast<double>(zVar)) : 0.0f;

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
                float floatValue = parsed.value.isDouble()
                    ? static_cast<float>(static_cast<double>(parsed.value))
                    : static_cast<float>(static_cast<int>(parsed.value));

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
                float x = xVar.isDouble() ? static_cast<float>(static_cast<double>(xVar)) : 0.0f;
                float y = yVar.isDouble() ? static_cast<float>(static_cast<double>(yVar)) : 0.0f;
                float z = zVar.isDouble() ? static_cast<float>(static_cast<double>(zVar)) : 0.0f;

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
                if (parsed.paramId == WFSParameterIDs::inputPositionX)
                    newValue = applyConstraintX(channelIndex, newValue);
                else if (parsed.paramId == WFSParameterIDs::inputPositionY)
                    newValue = applyConstraintY(channelIndex, newValue);
                else if (parsed.paramId == WFSParameterIDs::inputPositionZ)
                    newValue = applyConstraintZ(channelIndex, newValue);

                // Get current position values and apply distance constraint
                juce::var xVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionX);
                juce::var yVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionY);
                juce::var zVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputPositionZ);
                float x = xVar.isDouble() ? static_cast<float>(static_cast<double>(xVar)) : 0.0f;
                float y = yVar.isDouble() ? static_cast<float>(static_cast<double>(yVar)) : 0.0f;
                float z = zVar.isDouble() ? static_cast<float>(static_cast<double>(zVar)) : 0.0f;

                // Update with new value being set
                if (parsed.paramId == WFSParameterIDs::inputPositionX)
                    x = newValue;
                else if (parsed.paramId == WFSParameterIDs::inputPositionY)
                    y = newValue;
                else if (parsed.paramId == WFSParameterIDs::inputPositionZ)
                    z = newValue;

                // Apply distance constraint (modifies x, y, z in place)
                applyConstraintDistance(channelIndex, x, y, z);

                // Set all position values
                state.setInputParameter(channelIndex, WFSParameterIDs::inputPositionX, x);
                state.setInputParameter(channelIndex, WFSParameterIDs::inputPositionY, y);
                state.setInputParameter(channelIndex, WFSParameterIDs::inputPositionZ, z);
            }
            else
            {
                state.setInputParameter(channelIndex, parsed.paramId, newValue);
            }
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

                float currentX = posXVar.isDouble() ? static_cast<float>(static_cast<double>(posXVar)) :
                                (posXVar.isInt() ? static_cast<float>(static_cast<int>(posXVar)) : 0.0f);
                float currentY = posYVar.isDouble() ? static_cast<float>(static_cast<double>(posYVar)) :
                                (posYVar.isInt() ? static_cast<float>(static_cast<int>(posYVar)) : 0.0f);

                // Apply delta
                float newX = currentX + parsed.deltaX;
                float newY = currentY + parsed.deltaY;

                // Set new position
                state.setInputParameter(inputIndex, WFSParameterIDs::inputPositionX, newX);
                state.setInputParameter(inputIndex, WFSParameterIDs::inputPositionY, newY);
            }
        }

        incomingProtocol = Protocol::Disabled;  // Clear flag
    });
}

void OSCManager::sendRemoteChannelDump(int channelId)
{
    // Helper lambda to get a parameter value as float
    auto getParam = [this, channelId](const juce::Identifier& paramId) -> float {
        juce::var val = state.getInputParameter(channelId, paramId);
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
    paramValues[WFSParameterIDs::inputCluster] = getParam(WFSParameterIDs::inputCluster);
    paramValues[WFSParameterIDs::inputMaxSpeedActive] = getParam(WFSParameterIDs::inputMaxSpeedActive);
    paramValues[WFSParameterIDs::inputMaxSpeed] = getParam(WFSParameterIDs::inputMaxSpeed);
    paramValues[WFSParameterIDs::inputPathModeActive] = getParam(WFSParameterIDs::inputPathModeActive);
    paramValues[WFSParameterIDs::inputHeightFactor] = getParam(WFSParameterIDs::inputHeightFactor);

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

    // Build and send messages
    auto messages = OSCMessageBuilder::buildRemoteChannelDump(channelId, paramValues);

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

    // Get input count from IO section (can't use getIntParameter as "inputChannels" starts with "input"
    // and would be incorrectly routed to the Input channel scope instead of Config/IO)
    auto ioTree = state.getIOState();
    int inputs = ioTree.isValid()
        ? static_cast<int>(ioTree.getProperty(WFSParameterIDs::inputChannels))
        : 8;

    // Build messages
    std::vector<juce::OSCMessage> messages;
    messages.push_back(OSCMessageBuilder::buildConfigFloatMessage("/stage/originX", originX));
    messages.push_back(OSCMessageBuilder::buildConfigFloatMessage("/stage/originY", originY));
    messages.push_back(OSCMessageBuilder::buildConfigFloatMessage("/stage/originZ", originZ));
    messages.push_back(OSCMessageBuilder::buildConfigFloatMessage("/stage/width", width));
    messages.push_back(OSCMessageBuilder::buildConfigFloatMessage("/stage/depth", depth));
    messages.push_back(OSCMessageBuilder::buildConfigFloatMessage("/stage/height", height));
    messages.push_back(OSCMessageBuilder::buildConfigIntMessage("/stage/shape", shape));
    messages.push_back(OSCMessageBuilder::buildConfigFloatMessage("/stage/diameter", diameter));
    messages.push_back(OSCMessageBuilder::buildConfigIntMessage("/inputs", inputs));

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

void OSCManager::updateTargetStatus(int targetIndex, ConnectionStatus newStatus)
{
    if (targetIndex < 0 || targetIndex >= MAX_TARGETS)
        return;

    targetStatuses[static_cast<size_t>(targetIndex)] = newStatus;

    // Send stage config when Remote target connects
    if (newStatus == ConnectionStatus::Connected &&
        targetConfigs[static_cast<size_t>(targetIndex)].protocol == Protocol::Remote)
    {
        sendStageConfigToRemote();
    }

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
        return juce::jlimit(getStageMinX(), getStageMaxX(), value);

    return value;
}

float OSCManager::applyConstraintY(int channelIndex, float value) const
{
    juce::var constraintVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputConstraintY);
    int constraint = constraintVar.isInt() ? static_cast<int>(constraintVar) : 1;

    if (constraint != 0)
        return juce::jlimit(getStageMinY(), getStageMaxY(), value);

    return value;
}

float OSCManager::applyConstraintZ(int channelIndex, float value) const
{
    juce::var constraintVar = state.getInputParameter(channelIndex, WFSParameterIDs::inputConstraintZ);
    int constraint = constraintVar.isInt() ? static_cast<int>(constraintVar) : 1;

    if (constraint != 0)
        return juce::jlimit(0.0f, getStageMaxZ(), value);

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
    float minDist = minDistVar.isDouble() ? static_cast<float>(static_cast<double>(minDistVar)) : 0.0f;
    float maxDist = maxDistVar.isDouble() ? static_cast<float>(static_cast<double>(maxDistVar)) : 50.0f;

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

} // namespace WFSNetwork
