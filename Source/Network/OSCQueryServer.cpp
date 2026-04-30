#include "OSCQueryServer.h"
#include "OSCMessageRouter.h"
#include "OSCProtocolTypes.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"

namespace WFSNetwork
{

//==============================================================================
// Construction / Destruction
//==============================================================================

OSCQueryServer::OSCQueryServer(WFSValueTreeState& stateRef)
    : state(stateRef)
    , stateTree(stateRef.getState())
{
    stateTree.addListener(static_cast<juce::ValueTree::Listener*>(this));
}

OSCQueryServer::~OSCQueryServer()
{
    stop();
    stateTree.removeListener(static_cast<juce::ValueTree::Listener*>(this));
}

//==============================================================================
// Start / Stop
//==============================================================================

bool OSCQueryServer::start(int oscPortParam, int httpPortParam)
{
    if (running.load())
        stop();

    oscPort = oscPortParam;
    httpPort = httpPortParam;

    wsServer = std::make_unique<SimpleWebSocketServer>();
    wsServer->addHTTPRequestHandler(this);
    wsServer->addWebSocketListener(this);
    wsServer->start(httpPort);

    running = true;
    startTimer(30);  // Flush pending WebSocket pushes every 30ms
    DBG("OSCQueryServer: Started on port " << httpPort << " (OSC port " << oscPort << ")");

    DBG("OSCQueryServer: ValueTree type='" << stateTree.getType().toString()
        << "' children=" << stateTree.getNumChildren());

    return true;
}

void OSCQueryServer::beginIncomingOSC(const juce::String& senderIP)
{
    const juce::ScopedLock sl(senderIPLock);
    lastOSCSenderIP = senderIP;
    if (lastOSCSenderIP.startsWith("::ffff:"))
        lastOSCSenderIP = lastOSCSenderIP.substring(7);
    suppressIP = true;
}

void OSCQueryServer::endIncomingOSC()
{
    suppressIP = false;
}

void OSCQueryServer::stop()
{
    if (!running.load())
        return;

    running = false;
    stopTimer();

    if (wsServer)
    {
        wsServer->removeWebSocketListener(this);
        wsServer->removeHTTPRequestHandler(this);
        wsServer->stop();
        wsServer.reset();
    }

    // Clear subscriptions
    {
        const juce::ScopedLock sl(subscriptionLock);
        subscriptions.clear();
    }

    DBG("OSCQueryServer: Stopped");
}

//==============================================================================
// HTTP Request Handler
//==============================================================================

bool OSCQueryServer::handleHTTPRequest(std::shared_ptr<HttpServer::Response> response,
                                        std::shared_ptr<HttpServer::Request> request)
{
    juce::String path = juce::String(request->path);
    juce::String query = juce::String(request->query_string);

    // Normalize path
    if (path.isEmpty())
        path = "/";
    if (path.length() > 1 && path.endsWithChar('/'))
        path = path.dropLastCharacters(1);

    // HOST_INFO query
    if (query == "HOST_INFO")
    {
        sendJsonResponse(response, 200, buildHostInfoJson());
        return true;
    }

    // Build full tree and walk to requested path
    juce::DynamicObject::Ptr rootPtr(buildFullTree());
    juce::DynamicObject::Ptr targetPtr = rootPtr;

    if (path != "/" && path.isNotEmpty())
    {
        auto segments = juce::StringArray::fromTokens(path.substring(1), "/", "");

        for (const auto& seg : segments)
        {
            const auto& contentsVar = targetPtr->getProperty("CONTENTS");
            auto* contentsObj = contentsVar.getDynamicObject();
            if (contentsObj == nullptr)
            {
                sendJsonResponse(response, 404,
                    "{\"ERROR\": \"Path not found: " + path + "\"}");
                return true;
            }

            const auto& childVar = contentsObj->getProperty(juce::Identifier(seg));
            auto* childObj = childVar.getDynamicObject();
            if (childObj == nullptr)
            {
                sendJsonResponse(response, 404,
                    "{\"ERROR\": \"Path not found: " + path + "\"}");
                return true;
            }

            targetPtr = childObj;
        }
    }

    // Attribute query (?VALUE, ?RANGE, ?TYPE, ?ACCESS, ?DESCRIPTION, ?CLIPMODE)
    if (query.isNotEmpty())
    {
        juce::String attr = query.toUpperCase();

        if (attr == "VALUE" || attr == "TYPE" || attr == "RANGE" ||
            attr == "ACCESS" || attr == "DESCRIPTION" || attr == "CLIPMODE")
        {
            juce::String result = extractAttribute(targetPtr.get(), attr);
            if (result.isNotEmpty())
                sendJsonResponse(response, 200, result);
            else
                sendJsonResponse(response, 204, "");
            return true;
        }

        sendJsonResponse(response, 400,
            "{\"ERROR\": \"Unrecognized attribute: " + query + "\"}");
        return true;
    }

    // Full node response
    juce::var targetVar(targetPtr.get());
    sendJsonResponse(response, 200, juce::JSON::toString(targetVar, false));
    return true;
}

void OSCQueryServer::sendJsonResponse(std::shared_ptr<HttpServer::Response> response,
                                       int statusCode, const juce::String& body)
{
    SimpleWeb::StatusCode code;
    switch (statusCode)
    {
        case 200: code = SimpleWeb::StatusCode::success_ok; break;
        case 204: code = SimpleWeb::StatusCode::success_no_content; break;
        case 400: code = SimpleWeb::StatusCode::client_error_bad_request; break;
        case 404: code = SimpleWeb::StatusCode::client_error_not_found; break;
        default:  code = SimpleWeb::StatusCode::server_error_internal_server_error; break;
    }

    SimpleWeb::CaseInsensitiveMultimap header;
    header.emplace("Content-Type", "application/json");
    header.emplace("Access-Control-Allow-Origin", "*");

    response->write(code, body.toStdString(), header);
}

//==============================================================================
// WebSocket Listener — LISTEN/IGNORE
//==============================================================================

void OSCQueryServer::connectionOpened(const juce::String& id)
{
    DBG("OSCQueryServer: WebSocket connection opened: " << id);
}

void OSCQueryServer::messageReceived(const juce::String& id, const juce::String& message)
{
    // Parse JSON command: {"COMMAND":"LISTEN","DATA":"/wfs/input/1/positionX"}
    auto json = juce::JSON::parse(message);

    if (auto* obj = json.getDynamicObject())
    {
        juce::String command = obj->getProperty("COMMAND").toString();
        juce::String data = obj->getProperty("DATA").toString();

        if (command == "LISTEN" && data.isNotEmpty())
            handleListenCommand(id, data);
        else if (command == "IGNORE" && data.isNotEmpty())
            handleIgnoreCommand(id, data);
        else
            DBG("OSCQueryServer: Unknown WS command: " << command);
    }
    else
    {
        DBG("OSCQueryServer: Failed to parse WS message as JSON");
    }
}

void OSCQueryServer::connectionClosed(const juce::String& id, int /*status*/, const juce::String& /*reason*/)
{
    DBG("OSCQueryServer: WebSocket connection closed: " << id);
    removeAllSubscriptions(id);
}

void OSCQueryServer::connectionError(const juce::String& id, const juce::String& errorMsg)
{
    DBG("OSCQueryServer: WebSocket error for " << id << ": " << errorMsg);
    removeAllSubscriptions(id);
}

//==============================================================================
// Subscription Management
//==============================================================================

void OSCQueryServer::handleListenCommand(const juce::String& connectionId, const juce::String& path)
{
    const juce::ScopedLock sl(subscriptionLock);

    auto& listeners = subscriptions[path];
    if (!listeners.contains(connectionId))
    {
        listeners.add(connectionId);
        DBG("OSCQueryServer: LISTEN " << path << " from " << connectionId);
    }
}

void OSCQueryServer::handleIgnoreCommand(const juce::String& connectionId, const juce::String& path)
{
    const juce::ScopedLock sl(subscriptionLock);

    auto it = subscriptions.find(path);
    if (it != subscriptions.end())
    {
        it->second.removeString(connectionId);
        if (it->second.isEmpty())
            subscriptions.erase(it);
        DBG("OSCQueryServer: IGNORE " << path << " from " << connectionId);
    }
}

void OSCQueryServer::removeAllSubscriptions(const juce::String& connectionId)
{
    const juce::ScopedLock sl(subscriptionLock);

    for (auto it = subscriptions.begin(); it != subscriptions.end(); )
    {
        it->second.removeString(connectionId);
        if (it->second.isEmpty())
            it = subscriptions.erase(it);
        else
            ++it;
    }
}

//==============================================================================
// Value Change Push (binary OSC via WebSocket)
//==============================================================================

void OSCQueryServer::pushValueChange(const juce::String& oscPath, const juce::var& value)
{
    if (!wsServer || !running.load())
        return;

    // Build raw OSC packet manually (address + type tag + argument, all 4-byte aligned)
    juce::MemoryOutputStream stream;

    // Address
    auto addressStr = oscPath.toStdString();
    stream.write(addressStr.c_str(), addressStr.size() + 1);
    // Pad to 4-byte boundary
    while (stream.getDataSize() % 4 != 0)
        stream.writeByte(0);

    // Type tag
    if (value.isInt() || value.isInt64())
    {
        stream.write(",i\0\0", 4);
        // Write int32 big-endian
        int32_t v = (int32_t)(int)value;
        uint8_t bytes[4] = {
            (uint8_t)((v >> 24) & 0xFF), (uint8_t)((v >> 16) & 0xFF),
            (uint8_t)((v >> 8) & 0xFF),  (uint8_t)(v & 0xFF)
        };
        stream.write(bytes, 4);
    }
    else if (value.isString())
    {
        stream.write(",s\0\0", 4);
        auto str = value.toString().toStdString();
        stream.write(str.c_str(), str.size() + 1);
        while (stream.getDataSize() % 4 != 0)
            stream.writeByte(0);
    }
    else
    {
        // Float
        stream.write(",f\0\0", 4);
        float fv = (float)value;
        uint32_t bits;
        std::memcpy(&bits, &fv, 4);
        uint8_t bytes[4] = {
            (uint8_t)((bits >> 24) & 0xFF), (uint8_t)((bits >> 16) & 0xFF),
            (uint8_t)((bits >> 8) & 0xFF),  (uint8_t)(bits & 0xFF)
        };
        stream.write(bytes, 4);
    }

    juce::MemoryBlock oscData(stream.getData(), stream.getDataSize());

    // Send to all subscribed connections, skipping the IP that sent the OSC
    juce::StringArray listeners;
    {
        const juce::ScopedLock sl(subscriptionLock);
        auto it = subscriptions.find(oscPath);
        if (it != subscriptions.end())
            listeners = it->second;
    }

    juce::String skipIP;
    if (suppressIP.load())
    {
        const juce::ScopedLock sl(senderIPLock);
        skipIP = lastOSCSenderIP;
    }

    for (const auto& connId : listeners)
    {
        // connId is "IP:port" — extract IP part and compare
        if (skipIP.isNotEmpty())
        {
            juce::String connIP = connId.upToLastOccurrenceOf(":", false, true);
            if (connIP.startsWith("::ffff:"))
                connIP = connIP.substring(7);
            if (connIP == skipIP)
                continue;  // Skip — this client sent the OSC that caused this change
        }
        wsServer->sendTo(oscData, connId);
    }
}

//==============================================================================
// Reverse Lookup: paramId -> OSC address name
//==============================================================================

const std::map<juce::Identifier, OSCQueryServer::ReverseEntry>& OSCQueryServer::getReverseMap()
{
    static std::map<juce::Identifier, ReverseEntry> reverseMap;
    static bool built = false;

    if (!built)
    {
        for (const auto& [name, id] : OSCMessageRouter::getInputAddressMap())
            reverseMap[id] = { name, "input" };
        for (const auto& [name, id] : OSCMessageRouter::getOutputAddressMap())
            reverseMap[id] = { name, "output" };
        for (const auto& [name, id] : OSCMessageRouter::getReverbAddressMap())
            reverseMap[id] = { name, "reverb" };
        // Config params use full paths — store differently
        for (const auto& [fullPath, id] : OSCMessageRouter::getConfigAddressMap())
            reverseMap[id] = { fullPath, "config" };
        built = true;
    }

    return reverseMap;
}

juce::String OSCQueryServer::resolveOSCPath(const juce::ValueTree& tree,
                                             const juce::Identifier& property) const
{
    const auto& reverseMap = getReverseMap();
    auto it = reverseMap.find(property);
    if (it == reverseMap.end())
        return {};

    const auto& entry = it->second;

    if (entry.category == "config")
    {
        // Config params use the full OSC path directly
        return entry.oscName;
    }

    // The tree hierarchy is: Root > Inputs/Outputs/Reverbs > Channel_N > SubSection > property
    // The `tree` parameter is the SubSection where the property lives.
    // Walk up to find the category container and channel index.
    auto current = tree;


    while (current.isValid())
    {
        auto parent = current.getParent();
        if (!parent.isValid())
            break;

        juce::String parentType = parent.getType().toString();
        if (parentType == "Inputs" || parentType == "Outputs" || parentType == "Reverbs")
        {
            int channelIndex = parent.indexOf(current);
            return "/wfs/" + entry.category + "/" + juce::String(channelIndex + 1) + "/" + entry.oscName;
        }

        current = parent;
    }

    return {};
}

//==============================================================================
// ValueTree::Listener — Push Changes
//==============================================================================

void OSCQueryServer::timerCallback()
{
    if (!running.load() || !wsServer)
        return;

    // Drain all pending pushes
    std::map<juce::String, PendingPush> toDrain;
    {
        const juce::ScopedLock sl(pendingPushLock);
        if (pendingPushes.empty())
            return;
        toDrain.swap(pendingPushes);
    }

    for (const auto& [path, pending] : toDrain)
        pushValueChange(pending.oscPath, pending.value);
}

void OSCQueryServer::valueTreePropertyChanged(juce::ValueTree& tree,
                                              const juce::Identifier& property)
{
    if (!running.load() || !wsServer)
        return;

    // Quick check: any subscriptions at all?
    {
        const juce::ScopedLock sl(subscriptionLock);
        if (subscriptions.empty())
            return;
    }

    // Resolve to OSC path
    juce::String oscPath = resolveOSCPath(tree, property);
    if (oscPath.isEmpty())
        return;

    // Check if anyone is subscribed to this path
    {
        const juce::ScopedLock sl(subscriptionLock);
        if (subscriptions.find(oscPath) == subscriptions.end())
            return;
    }

    // Accumulate for throttled push — latest value per path wins
    juce::var value = tree.getProperty(property);
    {
        const juce::ScopedLock sl(pendingPushLock);
        pendingPushes[oscPath] = { oscPath, value };
    }
}

void OSCQueryServer::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& /*child*/)
{
    if (!running.load() || !wsServer)
        return;

    // Notify all WebSocket clients of structure change
    juce::String parentType = parent.getType().toString();
    juce::String pathChanged;
    if (parentType == "Inputs")       pathChanged = "/wfs/input";
    else if (parentType == "Outputs") pathChanged = "/wfs/output";
    else if (parentType == "Reverbs") pathChanged = "/wfs/reverb";

    if (pathChanged.isNotEmpty())
    {
        auto* cmd = new juce::DynamicObject();
        cmd->setProperty("COMMAND", "PATH_CHANGED");
        cmd->setProperty("DATA", pathChanged);
        wsServer->send(juce::JSON::toString(juce::var(cmd), false));
    }
}

void OSCQueryServer::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& /*child*/, int /*index*/)
{
    if (!running.load() || !wsServer)
        return;

    juce::String parentType = parent.getType().toString();
    juce::String pathChanged;
    if (parentType == "Inputs")       pathChanged = "/wfs/input";
    else if (parentType == "Outputs") pathChanged = "/wfs/output";
    else if (parentType == "Reverbs") pathChanged = "/wfs/reverb";

    if (pathChanged.isNotEmpty())
    {
        auto* cmd = new juce::DynamicObject();
        cmd->setProperty("COMMAND", "PATH_CHANGED");
        cmd->setProperty("DATA", pathChanged);
        wsServer->send(juce::JSON::toString(juce::var(cmd), false));
    }
}

//==============================================================================
// HOST_INFO
//==============================================================================

juce::String OSCQueryServer::buildHostInfoJson()
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("NAME", "WFS-DIY");
    obj->setProperty("OSC_PORT", oscPort);
    obj->setProperty("OSC_TRANSPORT", "UDP");
    obj->setProperty("WS_PORT", httpPort);  // Same port for HTTP and WS

    auto* ext = new juce::DynamicObject();
    ext->setProperty("ACCESS", true);
    ext->setProperty("VALUE", true);
    ext->setProperty("RANGE", true);
    ext->setProperty("DESCRIPTION", true);
    ext->setProperty("TYPE", true);
    ext->setProperty("CLIPMODE", true);
    ext->setProperty("LISTEN", true);
    obj->setProperty("EXTENSIONS", juce::var(ext));

    return juce::JSON::toString(juce::var(obj), false);
}

//==============================================================================
// Helpers
//==============================================================================

juce::DynamicObject* OSCQueryServer::makeContainerNode(const juce::String& fullPath,
                                                         const juce::String& description)
{
    auto* node = new juce::DynamicObject();
    node->setProperty("FULL_PATH", fullPath);
    node->setProperty("ACCESS", 0);
    node->setProperty("DESCRIPTION", description);
    node->setProperty("CONTENTS", juce::var(new juce::DynamicObject()));
    return node;
}

juce::DynamicObject* OSCQueryServer::makeParamNode(const juce::String& fullPath,
                                                     const juce::String& type,
                                                     const juce::var& value,
                                                     float rangeMin, float rangeMax,
                                                     const juce::String& description)
{
    auto* node = new juce::DynamicObject();
    node->setProperty("FULL_PATH", fullPath);
    node->setProperty("TYPE", type);
    node->setProperty("ACCESS", 3);

    juce::Array<juce::var> valueArr;
    valueArr.add(value);
    node->setProperty("VALUE", valueArr);

    if (rangeMin != rangeMax) // sentinel: equal means no range
    {
        auto* rangeObj = new juce::DynamicObject();
        rangeObj->setProperty("MIN", rangeMin);
        rangeObj->setProperty("MAX", rangeMax);
        juce::Array<juce::var> rangeArr;
        rangeArr.add(juce::var(rangeObj));
        node->setProperty("RANGE", rangeArr);

        juce::Array<juce::var> clipArr;
        clipArr.add("both");
        node->setProperty("CLIPMODE", clipArr);
    }

    node->setProperty("DESCRIPTION", description);
    return node;
}

juce::String OSCQueryServer::getOSCTypeTag(const juce::var& value)
{
    if (value.isInt() || value.isInt64())
        return "i";
    if (value.isString())
        return "s";
    return "f";
}

bool OSCQueryServer::isEQParam(const juce::String& oscName)
{
    return oscName.startsWith("EQ") && oscName != "EQenable"
        && oscName != "EQenabled";
}

//==============================================================================
// Parameter Range Lookup
//==============================================================================

OSCQueryServer::ParamRange OSCQueryServer::getParamRange(const juce::Identifier& paramId)
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    // --- Input Channel ---
    if (paramId == inputAttenuation)         return { inputAttenuationMin, inputAttenuationMax, true };
    if (paramId == inputDelayLatency)        return { inputDelayLatencyMin, inputDelayLatencyMax, true };
    if (paramId == inputMinimalLatency)      return { 0, 1, true };
    if (paramId == inputPositionX || paramId == inputPositionY || paramId == inputPositionZ)
        return { inputPositionMin, inputPositionMax, true };
    if (paramId == inputOffsetX || paramId == inputOffsetY || paramId == inputOffsetZ)
        return { inputOffsetMin, inputOffsetMax, true };
    if (paramId == inputConstraintX || paramId == inputConstraintY || paramId == inputConstraintZ)
        return { 0, 1, true };
    if (paramId == inputConstraintDistance)   return { 0, 1, true };
    if (paramId == inputConstraintDistanceMin) return { 0.0f, 50.0f, true };
    if (paramId == inputConstraintDistanceMax) return { 0.0f, 50.0f, true };
    if (paramId == inputFlipX || paramId == inputFlipY || paramId == inputFlipZ)
        return { 0, 1, true };
    if (paramId == inputCluster)             return { (float)inputClusterMin, (float)inputClusterMax, true };
    if (paramId == inputTrackingActive)      return { 0, 1, true };
    if (paramId == inputTrackingID)          return { (float)inputTrackingIDMin, (float)inputTrackingIDMax, true };
    if (paramId == inputTrackingSmooth)      return { (float)inputTrackingSmoothMin, (float)inputTrackingSmoothMax, true };
    if (paramId == inputMaxSpeedActive)      return { 0, 1, true };
    if (paramId == inputMaxSpeed)            return { inputMaxSpeedMin, inputMaxSpeedMax, true };
    if (paramId == inputPathModeActive)      return { 0, 1, true };
    if (paramId == inputHeightFactor)        return { (float)inputHeightFactorMin, (float)inputHeightFactorMax, true };

    if (paramId == inputAttenuationLaw)      return { 0, 1, true };
    if (paramId == inputDistanceAttenuation) return { inputDistanceAttenuationMin, inputDistanceAttenuationMax, true };
    if (paramId == inputDistanceRatio)       return { inputDistanceRatioMin, inputDistanceRatioMax, true };
    if (paramId == inputCommonAtten)         return { (float)inputCommonAttenMin, (float)inputCommonAttenMax, true };

    if (paramId == inputDirectivity)         return { (float)inputDirectivityMin, (float)inputDirectivityMax, true };
    if (paramId == inputRotation)            return { (float)inputRotationMin, (float)inputRotationMax, true };
    if (paramId == inputTilt)                return { (float)inputTiltMin, (float)inputTiltMax, true };
    if (paramId == inputHFshelf)             return { inputHFshelfMin, inputHFshelfMax, true };

    if (paramId == inputLSactive)            return { 0, 1, true };
    if (paramId == inputLSradius)            return { inputLSradiusMin, inputLSradiusMax, true };
    if (paramId == inputLSshape)             return { (float)inputLSshapeMin, (float)inputLSshapeMax, true };
    if (paramId == inputLSattenuation)       return { inputLSattenuationMin, inputLSattenuationMax, true };
    if (paramId == inputLSpeakThreshold)     return { inputLSpeakThresholdMin, inputLSpeakThresholdMax, true };
    if (paramId == inputLSpeakRatio)         return { inputLSpeakRatioMin, inputLSpeakRatioMax, true };
    if (paramId == inputLSslowThreshold)     return { inputLSslowThresholdMin, inputLSslowThresholdMax, true };
    if (paramId == inputLSslowRatio)         return { inputLSslowRatioMin, inputLSslowRatioMax, true };

    if (paramId == inputFRactive)            return { 0, 1, true };
    if (paramId == inputFRattenuation)       return { inputFRattenuationMin, inputFRattenuationMax, true };
    if (paramId == inputFRlowCutActive)      return { 0, 1, true };
    if (paramId == inputFRlowCutFreq)        return { (float)inputFRfreqMin, (float)inputFRfreqMax, true };
    if (paramId == inputFRhighShelfActive)   return { 0, 1, true };
    if (paramId == inputFRhighShelfFreq)     return { (float)inputFRfreqMin, (float)inputFRfreqMax, true };
    if (paramId == inputFRhighShelfGain)     return { inputFRhighShelfGainMin, inputFRhighShelfGainMax, true };
    if (paramId == inputFRhighShelfSlope)    return { inputFRhighShelfSlopeMin, inputFRhighShelfSlopeMax, true };
    if (paramId == inputFRdiffusion)         return { (float)inputFRdiffusionMin, (float)inputFRdiffusionMax, true };
    if (paramId == inputMuteReverbSends)     return { 0, 1, true };

    if (paramId == inputJitter)              return { inputJitterMin, inputJitterMax, true };

    if (paramId == inputLFOactive)           return { 0, 1, true };
    if (paramId == inputLFOperiod)           return { inputLFOperiodMin, inputLFOperiodMax, true };
    if (paramId == inputLFOphase)            return { (float)inputLFOphaseMin, (float)inputLFOphaseMax, true };
    if (paramId == inputLFOshapeX || paramId == inputLFOshapeY || paramId == inputLFOshapeZ)
        return { (float)inputLFOshapeMin, (float)inputLFOshapeMax, true };
    if (paramId == inputLFOrateX || paramId == inputLFOrateY || paramId == inputLFOrateZ)
        return { inputLFOrateMin, inputLFOrateMax, true };
    if (paramId == inputLFOamplitudeX || paramId == inputLFOamplitudeY || paramId == inputLFOamplitudeZ)
        return { inputLFOamplitudeMin, inputLFOamplitudeMax, true };
    if (paramId == inputLFOphaseX || paramId == inputLFOphaseY || paramId == inputLFOphaseZ)
        return { (float)inputLFOphaseMin, (float)inputLFOphaseMax, true };
    if (paramId == inputLFOgyrophone)        return { (float)inputLFOgyrophoneMin, (float)inputLFOgyrophoneMax, true };

    if (paramId == inputOtomoX || paramId == inputOtomoY || paramId == inputOtomoZ)
        return { inputOtomoMin, inputOtomoMax, true };
    if (paramId == inputOtomoAbsoluteRelative) return { 0, 1, true };
    if (paramId == inputOtomoStayReturn)     return { 0, 1, true };
    if (paramId == inputOtomoDuration)       return { inputOtomoDurationMin, inputOtomoDurationMax, true };
    if (paramId == inputOtomoCurve)          return { (float)inputOtomoCurveMin, (float)inputOtomoCurveMax, true };
    if (paramId == inputOtomoSpeedProfile)   return { (float)inputOtomoSpeedProfileMin, (float)inputOtomoSpeedProfileMax, true };
    if (paramId == inputOtomoTrigger)        return { 0, 1, true };
    if (paramId == inputOtomoThreshold)      return { inputOtomoThresholdMin, inputOtomoThresholdMax, true };
    if (paramId == inputOtomoReset)          return { inputOtomoResetMin, inputOtomoResetMax, true };
    if (paramId == inputOtomoPauseResume)    return { 0, 1, true };
    if (paramId == inputOtomoCoordinateMode) return { 0, 2, true };
    if (paramId == inputOtomoR)              return { inputOtomoRMin, inputOtomoRMax, true };
    if (paramId == inputOtomoTheta)          return { inputOtomoThetaMin, inputOtomoThetaMax, true };
    if (paramId == inputOtomoRsph)           return { inputOtomoRsphMin, inputOtomoRsphMax, true };
    if (paramId == inputOtomoPhi)            return { inputOtomoPhiMin, inputOtomoPhiMax, true };

    if (paramId == inputMuteMacro)           return { 0, 4, true };
    if (paramId == inputSidelinesActive)     return { 0, 1, true };
    if (paramId == inputSidelinesFringe)     return { inputSidelinesFringeMin, inputSidelinesFringeMax, true };
    if (paramId == inputReverbSend)          return { -92.0f, 0.0f, true };
    if (paramId == inputSamplerActive)       return { 0, 1, true };
    if (paramId == inputSamplerActiveSet)    return { 0, 16, true };
    if (paramId == gmLayer0Enabled || paramId == gmLayer1Enabled || paramId == gmLayer2Enabled)
        return { 0, 1, true };

    // --- Output Channel ---
    if (paramId == outputArray)              return { (float)outputArrayMin, (float)outputArrayMax, true };
    if (paramId == outputApplyToArray)       return { 0, 2, true };
    if (paramId == outputAttenuation)        return { outputAttenuationMin, outputAttenuationMax, true };
    if (paramId == outputDelayLatency)       return { outputDelayLatencyMin, outputDelayLatencyMax, true };
    if (paramId == outputPositionX || paramId == outputPositionY || paramId == outputPositionZ)
        return { outputPositionMin, outputPositionMax, true };
    if (paramId == outputOrientation)        return { (float)outputOrientationMin, (float)outputOrientationMax, true };
    if (paramId == outputAngleOn)            return { (float)outputAngleOnMin, (float)outputAngleOnMax, true };
    if (paramId == outputAngleOff)           return { (float)outputAngleOffMin, (float)outputAngleOffMax, true };
    if (paramId == outputPitch)              return { (float)outputPitchMin, (float)outputPitchMax, true };
    if (paramId == outputHFdamping)          return { outputHFdampingMin, outputHFdampingMax, true };
    if (paramId == outputMiniLatencyEnable)  return { 0, 1, true };
    if (paramId == outputLSattenEnable)      return { 0, 1, true };
    if (paramId == outputFRenable)           return { 0, 1, true };
    if (paramId == outputDistanceAttenPercent) return { (float)outputDistanceAttenPercentMin, (float)outputDistanceAttenPercentMax, true };
    if (paramId == outputHparallax)
        return { outputHparallaxMin, outputHparallaxMax, true };
    if (paramId == outputVparallax)
        return { outputVparallaxMin, outputVparallaxMax, true };
    if (paramId == outputEQenabled)          return { 0, 1, true };
    if (paramId == eqShape)                  return { (float)eqShapeMin, (float)eqShapeMax, true };
    if (paramId == eqFrequency)              return { eqFrequencyMin, eqFrequencyMax, true };
    if (paramId == eqGain)                   return { eqGainMin, eqGainMax, true };
    if (paramId == eqQ)                      return { eqQMin, eqQMax, true };
    if (paramId == eqSlope)                  return { eqSlopeMin, eqSlopeMax, true };

    // --- Reverb Channel ---
    if (paramId == reverbAttenuation)        return { reverbAttenuationMin, reverbAttenuationMax, true };
    if (paramId == reverbDelayLatency)       return { reverbDelayLatencyMin, reverbDelayLatencyMax, true };
    if (paramId == reverbPositionX || paramId == reverbPositionY || paramId == reverbPositionZ)
        return { reverbPositionMin, reverbPositionMax, true };
    if (paramId == reverbReturnOffsetX || paramId == reverbReturnOffsetY || paramId == reverbReturnOffsetZ)
        return { reverbReturnOffsetMin, reverbReturnOffsetMax, true };
    if (paramId == reverbOrientation)        return { (float)reverbOrientationMin, (float)reverbOrientationMax, true };
    if (paramId == reverbAngleOn)            return { (float)reverbAngleOnMin, (float)reverbAngleOnMax, true };
    if (paramId == reverbAngleOff)           return { (float)reverbAngleOffMin, (float)reverbAngleOffMax, true };
    if (paramId == reverbPitch)              return { (float)reverbPitchMin, (float)reverbPitchMax, true };
    if (paramId == reverbHFdamping)          return { reverbHFdampingMin, reverbHFdampingMax, true };
    if (paramId == reverbMiniLatencyEnable)  return { 0, 1, true };
    if (paramId == reverbLSenable)           return { 0, 1, true };
    if (paramId == reverbDistanceAttenEnable) return { (float)reverbDistanceAttenEnableMin, (float)reverbDistanceAttenEnableMax, true };
    if (paramId == reverbPreEQenable)        return { 0, 1, true };
    if (paramId == reverbPreEQshape)         return { (float)reverbPreEQshapeMin, (float)reverbPreEQshapeMax, true };
    if (paramId == reverbPreEQfreq)          return { (float)reverbPreEQfreqMin, (float)reverbPreEQfreqMax, true };
    if (paramId == reverbPreEQgain)          return { reverbPreEQgainMin, reverbPreEQgainMax, true };
    if (paramId == reverbPreEQq)             return { reverbPreEQqMin, reverbPreEQqMax, true };
    if (paramId == reverbPreEQslope)         return { reverbPreEQslopeMin, reverbPreEQslopeMax, true };
    if (paramId == reverbDistanceAttenuation) return { reverbDistanceAttenuationMin, reverbDistanceAttenuationMax, true };
    if (paramId == reverbCommonAtten)        return { (float)reverbCommonAttenMin, (float)reverbCommonAttenMax, true };
    if (paramId == reverbMuteMacro)          return { 0, 4, true };

    // --- Config: Stage ---
    if (paramId == stageShape)               return { (float)stageShapeMin, (float)stageShapeMax, true };
    if (paramId == stageWidth)               return { stageWidthMin, stageWidthMax, true };
    if (paramId == stageDepth)               return { stageDepthMin, stageDepthMax, true };
    if (paramId == stageHeight)              return { stageHeightMin, stageHeightMax, true };
    if (paramId == stageDiameter)            return { stageDiameterMin, stageDiameterMax, true };
    if (paramId == domeElevation)            return { domeElevationMin, domeElevationMax, true };
    if (paramId == originWidth)              return { originWidthMin, originWidthMax, true };
    if (paramId == originDepth)              return { originDepthMin, originDepthMax, true };
    if (paramId == originHeight)             return { originHeightMin, originHeightMax, true };

    // --- Config: Reverb Algorithm ---
    if (paramId == reverbAlgoType)           return { (float)reverbAlgoTypeMin, (float)reverbAlgoTypeMax, true };
    if (paramId == reverbRT60)               return { reverbRT60Min, reverbRT60Max, true };
    if (paramId == reverbRT60LowMult)        return { reverbRT60LowMultMin, reverbRT60LowMultMax, true };
    if (paramId == reverbRT60HighMult)       return { reverbRT60HighMultMin, reverbRT60HighMultMax, true };
    if (paramId == reverbCrossoverLow)       return { reverbCrossoverLowMin, reverbCrossoverLowMax, true };
    if (paramId == reverbCrossoverHigh)      return { reverbCrossoverHighMin, reverbCrossoverHighMax, true };
    if (paramId == reverbDiffusion)          return { reverbDiffusionMin, reverbDiffusionMax, true };
    if (paramId == reverbSDNscale)           return { reverbSDNscaleMin, reverbSDNscaleMax, true };
    if (paramId == reverbFDNsize)            return { reverbFDNsizeMin, reverbFDNsizeMax, true };
    if (paramId == reverbIRtrim)             return { reverbIRtrimMin, reverbIRtrimMax, true };
    if (paramId == reverbIRlength)           return { reverbIRlengthMin, reverbIRlengthMax, true };
    if (paramId == reverbPerNodeIR)          return { 0, 1, true };
    if (paramId == reverbWetLevel)           return { reverbWetLevelMin, reverbWetLevelMax, true };
    if (paramId == reverbPreCompBypass)      return { 0, 1, true };
    if (paramId == reverbPreCompThreshold)   return { reverbPreCompThresholdMin, reverbPreCompThresholdMax, true };
    if (paramId == reverbPreCompRatio)       return { reverbPreCompRatioMin, reverbPreCompRatioMax, true };
    if (paramId == reverbPreCompAttack)      return { reverbPreCompAttackMin, reverbPreCompAttackMax, true };
    if (paramId == reverbPreCompRelease)     return { reverbPreCompReleaseMin, reverbPreCompReleaseMax, true };
    if (paramId == reverbPostEQenable)       return { 0, 1, true };
    if (paramId == reverbPostEQshape)        return { (float)reverbPostEQshapeMin, (float)reverbPostEQshapeMax, true };
    if (paramId == reverbPostEQfreq)         return { (float)reverbPostEQfreqMin, (float)reverbPostEQfreqMax, true };
    if (paramId == reverbPostEQgain)         return { reverbPostEQgainMin, reverbPostEQgainMax, true };
    if (paramId == reverbPostEQq)            return { reverbPostEQqMin, reverbPostEQqMax, true };
    if (paramId == reverbPostEQslope)        return { reverbPostEQslopeMin, reverbPostEQslopeMax, true };
    if (paramId == reverbPostExpBypass)      return { 0, 1, true };
    if (paramId == reverbPostExpThreshold)   return { reverbPostExpThresholdMin, reverbPostExpThresholdMax, true };
    if (paramId == reverbPostExpRatio)       return { reverbPostExpRatioMin, reverbPostExpRatioMax, true };
    if (paramId == reverbPostExpAttack)      return { reverbPostExpAttackMin, reverbPostExpAttackMax, true };
    if (paramId == reverbPostExpRelease)     return { reverbPostExpReleaseMin, reverbPostExpReleaseMax, true };

    return { 0.0f, 0.0f, false };
}

//==============================================================================
// Attribute Extraction (?VALUE, ?TYPE, etc.)
//==============================================================================

juce::String OSCQueryServer::extractAttribute(juce::DynamicObject* node, const juce::String& attr)
{
    if (node == nullptr)
        return {};

    auto* result = new juce::DynamicObject();

    if (node->hasProperty("FULL_PATH"))
        result->setProperty("FULL_PATH", node->getProperty("FULL_PATH"));

    if (attr == "VALUE" && node->hasProperty("VALUE"))
        result->setProperty("VALUE", node->getProperty("VALUE"));
    else if (attr == "TYPE" && node->hasProperty("TYPE"))
        result->setProperty("TYPE", node->getProperty("TYPE"));
    else if (attr == "RANGE" && node->hasProperty("RANGE"))
        result->setProperty("RANGE", node->getProperty("RANGE"));
    else if (attr == "ACCESS" && node->hasProperty("ACCESS"))
        result->setProperty("ACCESS", node->getProperty("ACCESS"));
    else if (attr == "DESCRIPTION" && node->hasProperty("DESCRIPTION"))
        result->setProperty("DESCRIPTION", node->getProperty("DESCRIPTION"));
    else if (attr == "CLIPMODE" && node->hasProperty("CLIPMODE"))
        result->setProperty("CLIPMODE", node->getProperty("CLIPMODE"));
    else
    {
        delete result;
        return {};
    }

    return juce::JSON::toString(juce::var(result), false);
}

//==============================================================================
// Namespace Tree Building
//==============================================================================

juce::DynamicObject* OSCQueryServer::buildFullTree()
{
    auto* root = new juce::DynamicObject();
    root->setProperty("FULL_PATH", "/");
    root->setProperty("ACCESS", 0);
    root->setProperty("DESCRIPTION", "WFS-DIY Wave Field Synthesis");
    root->setProperty("OSC_PORT", oscPort);

    auto* wfs = new juce::DynamicObject();
    wfs->setProperty("FULL_PATH", "/wfs");
    wfs->setProperty("ACCESS", 0);
    wfs->setProperty("DESCRIPTION", "WFS Parameters");

    auto* wfsContents = new juce::DynamicObject();

    // /wfs/input (1-based channel numbers, matching standard OSC convention)
    {
        auto* container = makeContainerNode("/wfs/input", "Input Channels");
        auto* contents = container->getProperties()["CONTENTS"].getDynamicObject();
        int count = state.getNumInputChannels();
        for (int i = 0; i < count; ++i)
            contents->setProperty(juce::String(i + 1), juce::var(buildInputChannelJson(i)));
        wfsContents->setProperty("input", juce::var(container));
    }

    // /wfs/output
    {
        auto* container = makeContainerNode("/wfs/output", "Output Channels");
        auto* contents = container->getProperties()["CONTENTS"].getDynamicObject();
        int count = state.getNumOutputChannels();
        for (int i = 0; i < count; ++i)
            contents->setProperty(juce::String(i + 1), juce::var(buildOutputChannelJson(i)));
        wfsContents->setProperty("output", juce::var(container));
    }

    // /wfs/reverb
    {
        auto* container = makeContainerNode("/wfs/reverb", "Reverb Channels");
        auto* contents = container->getProperties()["CONTENTS"].getDynamicObject();
        int count = state.getNumReverbChannels();
        for (int i = 0; i < count; ++i)
            contents->setProperty(juce::String(i + 1), juce::var(buildReverbChannelJson(i)));
        wfsContents->setProperty("reverb", juce::var(container));
    }

    // /wfs/config
    wfsContents->setProperty("config", juce::var(buildConfigJson()));

    wfs->setProperty("CONTENTS", juce::var(wfsContents));

    auto* rootContents = new juce::DynamicObject();
    rootContents->setProperty("wfs", juce::var(wfs));
    root->setProperty("CONTENTS", juce::var(rootContents));

    return root;
}

juce::DynamicObject* OSCQueryServer::buildInputChannelJson(int channelIndex)
{
    juce::String basePath = "/wfs/input/" + juce::String(channelIndex + 1);
    auto* channel = makeContainerNode(basePath, "Input " + juce::String(channelIndex + 1));
    auto* contents = channel->getProperties()["CONTENTS"].getDynamicObject();

    const auto& addrMap = OSCMessageRouter::getInputAddressMap();

    for (const auto& [oscName, paramId] : addrMap)
    {
        juce::String fullPath = basePath + "/" + oscName;
        juce::var value = state.getParameter(paramId, channelIndex);
        juce::String typeTag = getOSCTypeTag(value);
        auto range = getParamRange(paramId);

        if (range.hasRange)
            contents->setProperty(oscName, juce::var(makeParamNode(fullPath, typeTag, value, range.min, range.max, oscName)));
        else
            contents->setProperty(oscName, juce::var(makeParamNode(fullPath, typeTag, value, 0, 0, oscName)));
    }

    return channel;
}

juce::DynamicObject* OSCQueryServer::buildOutputChannelJson(int channelIndex)
{
    juce::String basePath = "/wfs/output/" + juce::String(channelIndex + 1);
    auto* channel = makeContainerNode(basePath, "Output " + juce::String(channelIndex + 1));
    auto* contents = channel->getProperties()["CONTENTS"].getDynamicObject();

    const auto& addrMap = OSCMessageRouter::getOutputAddressMap();

    for (const auto& [oscName, paramId] : addrMap)
    {
        juce::String fullPath = basePath + "/" + oscName;

        if (isEQParam(oscName))
        {
            auto range = getParamRange(paramId);
            auto* node = new juce::DynamicObject();
            node->setProperty("FULL_PATH", fullPath);
            node->setProperty("TYPE", "if");
            node->setProperty("ACCESS", 3);
            node->setProperty("DESCRIPTION", oscName + " (first arg: band index 0-5)");
            if (range.hasRange)
            {
                auto* rangeObj0 = new juce::DynamicObject();
                rangeObj0->setProperty("MIN", 0);
                rangeObj0->setProperty("MAX", WFSParameterDefaults::numEQBands - 1);
                auto* rangeObj1 = new juce::DynamicObject();
                rangeObj1->setProperty("MIN", range.min);
                rangeObj1->setProperty("MAX", range.max);
                juce::Array<juce::var> rangeArr;
                rangeArr.add(juce::var(rangeObj0));
                rangeArr.add(juce::var(rangeObj1));
                node->setProperty("RANGE", rangeArr);
            }
            contents->setProperty(oscName, juce::var(node));
        }
        else
        {
            juce::var value = state.getParameter(paramId, channelIndex);
            juce::String typeTag = getOSCTypeTag(value);
            auto range = getParamRange(paramId);

            if (range.hasRange)
                contents->setProperty(oscName, juce::var(makeParamNode(fullPath, typeTag, value, range.min, range.max, oscName)));
            else
                contents->setProperty(oscName, juce::var(makeParamNode(fullPath, typeTag, value, 0, 0, oscName)));
        }
    }

    return channel;
}

juce::DynamicObject* OSCQueryServer::buildReverbChannelJson(int channelIndex)
{
    juce::String basePath = "/wfs/reverb/" + juce::String(channelIndex + 1);
    auto* channel = makeContainerNode(basePath, "Reverb " + juce::String(channelIndex + 1));
    auto* contents = channel->getProperties()["CONTENTS"].getDynamicObject();

    const auto& addrMap = OSCMessageRouter::getReverbAddressMap();

    for (const auto& [oscName, paramId] : addrMap)
    {
        juce::String fullPath = basePath + "/" + oscName;

        if (oscName.startsWith("preEQ") && oscName != "preEQenable")
        {
            auto range = getParamRange(paramId);
            auto* node = new juce::DynamicObject();
            node->setProperty("FULL_PATH", fullPath);
            node->setProperty("TYPE", "if");
            node->setProperty("ACCESS", 3);
            node->setProperty("DESCRIPTION", oscName + " (first arg: band index 0-3)");
            if (range.hasRange)
            {
                auto* rangeObj0 = new juce::DynamicObject();
                rangeObj0->setProperty("MIN", 0);
                rangeObj0->setProperty("MAX", WFSParameterDefaults::numReverbPreEQBands - 1);
                auto* rangeObj1 = new juce::DynamicObject();
                rangeObj1->setProperty("MIN", range.min);
                rangeObj1->setProperty("MAX", range.max);
                juce::Array<juce::var> rangeArr;
                rangeArr.add(juce::var(rangeObj0));
                rangeArr.add(juce::var(rangeObj1));
                node->setProperty("RANGE", rangeArr);
            }
            contents->setProperty(oscName, juce::var(node));
        }
        else
        {
            juce::var value = state.getParameter(paramId, channelIndex);
            juce::String typeTag = getOSCTypeTag(value);
            auto range = getParamRange(paramId);

            if (range.hasRange)
                contents->setProperty(oscName, juce::var(makeParamNode(fullPath, typeTag, value, range.min, range.max, oscName)));
            else
                contents->setProperty(oscName, juce::var(makeParamNode(fullPath, typeTag, value, 0, 0, oscName)));
        }
    }

    return channel;
}

juce::DynamicObject* OSCQueryServer::buildConfigJson()
{
    auto* configContainer = makeContainerNode("/wfs/config", "System Configuration");
    auto* configContents = configContainer->getProperties()["CONTENTS"].getDynamicObject();

    const auto& addrMap = OSCMessageRouter::getConfigAddressMap();

    std::map<juce::String, juce::var> subContainers;

    for (const auto& [fullOscPath, paramId] : addrMap)
    {
        juce::String subPath = fullOscPath.fromFirstOccurrenceOf("/wfs/config/", false, true);
        if (subPath.isEmpty())
            continue;

        int slashIdx = subPath.indexOf("/");
        if (slashIdx < 0)
            continue;

        juce::String group = subPath.substring(0, slashIdx);
        juce::String paramName = subPath.substring(slashIdx + 1);

        if (subContainers.find(group) == subContainers.end())
            subContainers[group] = juce::var(makeContainerNode("/wfs/config/" + group, group + " parameters"));

        auto* subObj = subContainers[group].getDynamicObject();
        auto* subContents = subObj->getProperties()["CONTENTS"].getDynamicObject();

        juce::var value = state.getParameter(paramId, -1);
        juce::String typeTag = getOSCTypeTag(value);
        auto range = getParamRange(paramId);

        bool isBandParam = paramName.startsWith("postEQ") && paramName != "postEQenable";
        if (isBandParam)
        {
            auto* node = new juce::DynamicObject();
            node->setProperty("FULL_PATH", fullOscPath);
            node->setProperty("TYPE", "if");
            node->setProperty("ACCESS", 3);
            node->setProperty("DESCRIPTION", paramName + " (first arg: band index 0-3)");
            if (range.hasRange)
            {
                auto* rangeObj0 = new juce::DynamicObject();
                rangeObj0->setProperty("MIN", 0);
                rangeObj0->setProperty("MAX", WFSParameterDefaults::numReverbPostEQBands - 1);
                auto* rangeObj1 = new juce::DynamicObject();
                rangeObj1->setProperty("MIN", range.min);
                rangeObj1->setProperty("MAX", range.max);
                juce::Array<juce::var> rangeArr;
                rangeArr.add(juce::var(rangeObj0));
                rangeArr.add(juce::var(rangeObj1));
                node->setProperty("RANGE", rangeArr);
            }
            subContents->setProperty(paramName, juce::var(node));
        }
        else
        {
            if (range.hasRange)
                subContents->setProperty(paramName, juce::var(makeParamNode(fullOscPath, typeTag, value, range.min, range.max, paramName)));
            else
                subContents->setProperty(paramName, juce::var(makeParamNode(fullOscPath, typeTag, value, 0, 0, paramName)));
        }
    }

    for (auto& [group, subVar] : subContainers)
        configContents->setProperty(group, subVar);

    return configContainer;
}

} // namespace WFSNetwork
