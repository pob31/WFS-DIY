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
    : Thread("OSCQueryServer")
    , state(stateRef)
{
    state.getState().addListener(this);
}

OSCQueryServer::~OSCQueryServer()
{
    stop();
    state.getState().removeListener(this);
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

    serverSocket = std::make_unique<juce::StreamingSocket>();

    if (!serverSocket->createListener(httpPort))
    {
        DBG("OSCQueryServer: Failed to create listener on port " << httpPort);
        serverSocket.reset();
        return false;
    }

    running = true;
    startThread();
    return true;
}

void OSCQueryServer::stop()
{
    if (!running.load())
        return;

    running = false;

    if (serverSocket)
        serverSocket->close();

    stopThread(2000);
    serverSocket.reset();
}

//==============================================================================
// Thread
//==============================================================================

void OSCQueryServer::run()
{
    while (!threadShouldExit() && running.load())
    {
        if (serverSocket->waitUntilReady(true, 100) == 1)
        {
            auto* clientSocket = serverSocket->waitForNextConnection();
            if (clientSocket != nullptr)
            {
                std::unique_ptr<juce::StreamingSocket> client(clientSocket);
                handleHttpRequest(*client);
            }
        }
    }
}

//==============================================================================
// HTTP Request Handling
//==============================================================================

OSCQueryServer::ParsedRequest OSCQueryServer::parseHttpRequest(const juce::String& request)
{
    ParsedRequest result;
    result.path = "/";

    if (!request.startsWith("GET "))
        return result;

    int pathStart = 4;
    int pathEnd = request.indexOf(pathStart, " ");
    if (pathEnd < 0)
        pathEnd = request.indexOf(pathStart, "\r");
    if (pathEnd < 0)
        return result;

    juce::String uri = request.substring(pathStart, pathEnd);

    // Strip fragment
    int fragIdx = uri.indexOf("#");
    if (fragIdx >= 0)
        uri = uri.substring(0, fragIdx);

    // Split path and query
    int queryIdx = uri.indexOf("?");
    if (queryIdx >= 0)
    {
        result.path = uri.substring(0, queryIdx);
        result.query = uri.substring(queryIdx + 1);
    }
    else
    {
        result.path = uri;
    }

    if (result.path.isEmpty())
        result.path = "/";

    // Remove trailing slash (except for root)
    if (result.path.length() > 1 && result.path.endsWithChar('/'))
        result.path = result.path.dropLastCharacters(1);

    return result;
}

void OSCQueryServer::handleHttpRequest(juce::StreamingSocket& client)
{
    // Wait up to 1 second for data to arrive
    if (client.waitUntilReady(true, 1000) != 1)
        return;

    char buffer[4096];
    int bytesRead = client.read(buffer, sizeof(buffer) - 1, false);

    if (bytesRead <= 0)
        return;

    buffer[bytesRead] = '\0';
    juce::String request(buffer);

    auto parsed = parseHttpRequest(request);

    // HOST_INFO query (path is irrelevant per spec)
    if (parsed.query == "HOST_INFO")
    {
        sendHttpResponse(client, 200, "OK", buildHostInfoJson());
        return;
    }

    // Build full tree
    juce::DynamicObject::Ptr rootPtr(buildFullTree());
    juce::DynamicObject::Ptr targetPtr = rootPtr;

    if (parsed.path != "/" && parsed.path.isNotEmpty())
    {
        auto segments = juce::StringArray::fromTokens(parsed.path.substring(1), "/", "");

        for (const auto& seg : segments)
        {
            const auto& contentsVar = targetPtr->getProperty("CONTENTS");
            auto* contentsObj = contentsVar.getDynamicObject();
            if (contentsObj == nullptr)
            {
                sendHttpResponse(client, 404, "Not Found",
                    "{\"ERROR\": \"Path not found: " + parsed.path + "\"}");
                return;
            }

            const auto& childVar = contentsObj->getProperty(juce::Identifier(seg));
            auto* childObj = childVar.getDynamicObject();
            if (childObj == nullptr)
            {
                sendHttpResponse(client, 404, "Not Found",
                    "{\"ERROR\": \"Path not found: " + parsed.path + "\"}");
                return;
            }

            targetPtr = childObj;
        }
    }

    // Attribute query (?VALUE, ?RANGE, ?TYPE, ?ACCESS, ?DESCRIPTION, ?CLIPMODE)
    if (parsed.query.isNotEmpty())
    {
        juce::String attr = parsed.query.toUpperCase();

        // Recognized attributes
        if (attr == "VALUE" || attr == "TYPE" || attr == "RANGE" ||
            attr == "ACCESS" || attr == "DESCRIPTION" || attr == "CLIPMODE")
        {
            juce::String result = extractAttribute(targetPtr.get(), attr);
            if (result.isNotEmpty())
                sendHttpResponse(client, 200, "OK", result);
            else
                sendHttpResponse(client, 204, "No Content", "");
            return;
        }

        // Unrecognized attribute
        sendHttpResponse(client, 400, "Bad Request",
            "{\"ERROR\": \"Unrecognized attribute: " + parsed.query + "\"}");
        return;
    }

    // Full node response
    juce::var targetVar(targetPtr.get());
    sendHttpResponse(client, 200, "OK", juce::JSON::toString(targetVar, false));
}

void OSCQueryServer::sendHttpResponse(juce::StreamingSocket& client, int statusCode,
                                       const juce::String& statusText, const juce::String& body)
{
    juce::String response;
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    response << "Content-Type: application/json\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Content-Length: " << (int)body.getNumBytesAsUTF8() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;

    client.write(response.toRawUTF8(), (int)response.getNumBytesAsUTF8());
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

    auto* ext = new juce::DynamicObject();
    ext->setProperty("ACCESS", true);
    ext->setProperty("VALUE", true);
    ext->setProperty("RANGE", true);
    ext->setProperty("DESCRIPTION", true);
    ext->setProperty("TYPE", true);
    ext->setProperty("CLIPMODE", true);
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

        // All parameters are clamped to their range
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

    // Attenuation law
    if (paramId == inputAttenuationLaw)      return { 0, 1, true };
    if (paramId == inputDistanceAttenuation) return { inputDistanceAttenuationMin, inputDistanceAttenuationMax, true };
    if (paramId == inputDistanceRatio)       return { inputDistanceRatioMin, inputDistanceRatioMax, true };
    if (paramId == inputCommonAtten)         return { (float)inputCommonAttenMin, (float)inputCommonAttenMax, true };

    // Directivity
    if (paramId == inputDirectivity)         return { (float)inputDirectivityMin, (float)inputDirectivityMax, true };
    if (paramId == inputRotation)            return { (float)inputRotationMin, (float)inputRotationMax, true };
    if (paramId == inputTilt)                return { (float)inputTiltMin, (float)inputTiltMax, true };
    if (paramId == inputHFshelf)             return { inputHFshelfMin, inputHFshelfMax, true };

    // Live Source Tamer
    if (paramId == inputLSactive)            return { 0, 1, true };
    if (paramId == inputLSradius)            return { inputLSradiusMin, inputLSradiusMax, true };
    if (paramId == inputLSshape)             return { (float)inputLSshapeMin, (float)inputLSshapeMax, true };
    if (paramId == inputLSattenuation)       return { inputLSattenuationMin, inputLSattenuationMax, true };
    if (paramId == inputLSpeakThreshold)     return { inputLSpeakThresholdMin, inputLSpeakThresholdMax, true };
    if (paramId == inputLSpeakRatio)         return { inputLSpeakRatioMin, inputLSpeakRatioMax, true };
    if (paramId == inputLSslowThreshold)     return { inputLSslowThresholdMin, inputLSslowThresholdMax, true };
    if (paramId == inputLSslowRatio)         return { inputLSslowRatioMin, inputLSslowRatioMax, true };

    // Floor Reflections
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

    // Jitter
    if (paramId == inputJitter)              return { inputJitterMin, inputJitterMax, true };

    // LFO
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

    // AutomOtion
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

    // Mutes
    if (paramId == inputMuteMacro)           return { 0, 4, true };
    // inputMutes is a string -> no range

    // Sidelines
    if (paramId == inputSidelinesActive)     return { 0, 1, true };
    if (paramId == inputSidelinesFringe)     return { inputSidelinesFringeMin, inputSidelinesFringeMax, true };

    // Reverb send
    if (paramId == inputReverbSend)          return { -92.0f, 0.0f, true };

    // Sampler
    if (paramId == inputSamplerActive)       return { 0, 1, true };
    if (paramId == inputSamplerActiveSet)    return { 0, 16, true };

    // Gradient Map layers
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
    if (paramId == outputHparallax || paramId == outputVparallax)
        return { outputParallaxMin, outputParallaxMax, true };
    if (paramId == outputEQenabled)          return { 0, 1, true };

    // Output EQ band params
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

    // Reverb Pre-EQ
    if (paramId == reverbPreEQenable)        return { 0, 1, true };
    if (paramId == reverbPreEQshape)         return { (float)reverbPreEQshapeMin, (float)reverbPreEQshapeMax, true };
    if (paramId == reverbPreEQfreq)          return { (float)reverbPreEQfreqMin, (float)reverbPreEQfreqMax, true };
    if (paramId == reverbPreEQgain)          return { reverbPreEQgainMin, reverbPreEQgainMax, true };
    if (paramId == reverbPreEQq)             return { reverbPreEQqMin, reverbPreEQqMax, true };
    if (paramId == reverbPreEQslope)         return { reverbPreEQslopeMin, reverbPreEQslopeMax, true };

    // Reverb Return
    if (paramId == reverbDistanceAttenuation) return { reverbDistanceAttenuationMin, reverbDistanceAttenuationMax, true };
    if (paramId == reverbCommonAtten)        return { (float)reverbCommonAttenMin, (float)reverbCommonAttenMax, true };
    if (paramId == reverbMuteMacro)          return { 0, 4, true };
    // reverbMutes is a string -> no range

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

    // Config: Reverb Pre-Compressor
    if (paramId == reverbPreCompBypass)      return { 0, 1, true };
    if (paramId == reverbPreCompThreshold)   return { reverbPreCompThresholdMin, reverbPreCompThresholdMax, true };
    if (paramId == reverbPreCompRatio)       return { reverbPreCompRatioMin, reverbPreCompRatioMax, true };
    if (paramId == reverbPreCompAttack)      return { reverbPreCompAttackMin, reverbPreCompAttackMax, true };
    if (paramId == reverbPreCompRelease)     return { reverbPreCompReleaseMin, reverbPreCompReleaseMax, true };

    // Config: Reverb Post-EQ
    if (paramId == reverbPostEQenable)       return { 0, 1, true };
    if (paramId == reverbPostEQshape)        return { (float)reverbPostEQshapeMin, (float)reverbPostEQshapeMax, true };
    if (paramId == reverbPostEQfreq)         return { (float)reverbPostEQfreqMin, (float)reverbPostEQfreqMax, true };
    if (paramId == reverbPostEQgain)         return { reverbPostEQgainMin, reverbPostEQgainMax, true };
    if (paramId == reverbPostEQq)            return { reverbPostEQqMin, reverbPostEQqMax, true };
    if (paramId == reverbPostEQslope)        return { reverbPostEQslopeMin, reverbPostEQslopeMax, true };

    // Config: Reverb Post-Expander
    if (paramId == reverbPostExpBypass)      return { 0, 1, true };
    if (paramId == reverbPostExpThreshold)   return { reverbPostExpThresholdMin, reverbPostExpThresholdMax, true };
    if (paramId == reverbPostExpRatio)       return { reverbPostExpRatioMin, reverbPostExpRatioMax, true };
    if (paramId == reverbPostExpAttack)      return { reverbPostExpAttackMin, reverbPostExpAttackMax, true };
    if (paramId == reverbPostExpRelease)     return { reverbPostExpReleaseMin, reverbPostExpReleaseMax, true };

    // No range known
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

    // Always include FULL_PATH for context
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
        return {};  // Attribute not present -> caller sends 204
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

    // /wfs/input
    {
        auto* container = makeContainerNode("/wfs/input", "Input Channels");
        auto* contents = container->getProperties()["CONTENTS"].getDynamicObject();
        int count = state.getNumInputChannels();
        for (int i = 0; i < count; ++i)
            contents->setProperty(juce::String(i), juce::var(buildInputChannelJson(i)));
        wfsContents->setProperty("input", juce::var(container));
    }

    // /wfs/output
    {
        auto* container = makeContainerNode("/wfs/output", "Output Channels");
        auto* contents = container->getProperties()["CONTENTS"].getDynamicObject();
        int count = state.getNumOutputChannels();
        for (int i = 0; i < count; ++i)
            contents->setProperty(juce::String(i), juce::var(buildOutputChannelJson(i)));
        wfsContents->setProperty("output", juce::var(container));
    }

    // /wfs/reverb
    {
        auto* container = makeContainerNode("/wfs/reverb", "Reverb Channels");
        auto* contents = container->getProperties()["CONTENTS"].getDynamicObject();
        int count = state.getNumReverbChannels();
        for (int i = 0; i < count; ++i)
            contents->setProperty(juce::String(i), juce::var(buildReverbChannelJson(i)));
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
    juce::String basePath = "/wfs/input/" + juce::String(channelIndex);
    auto* channel = makeContainerNode(basePath, "Input " + juce::String(channelIndex));
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
    juce::String basePath = "/wfs/output/" + juce::String(channelIndex);
    auto* channel = makeContainerNode(basePath, "Output " + juce::String(channelIndex));
    auto* contents = channel->getProperties()["CONTENTS"].getDynamicObject();

    const auto& addrMap = OSCMessageRouter::getOutputAddressMap();

    for (const auto& [oscName, paramId] : addrMap)
    {
        juce::String fullPath = basePath + "/" + oscName;

        // EQ band params take bandIndex as first OSC arg
        if (isEQParam(oscName))
        {
            // Expose as TYPE "if" (int band index + float value)
            auto range = getParamRange(paramId);
            auto* node = new juce::DynamicObject();
            node->setProperty("FULL_PATH", fullPath);
            node->setProperty("TYPE", "if");
            node->setProperty("ACCESS", 3);
            node->setProperty("DESCRIPTION", oscName + " (first arg: band index 0-5)");
            if (range.hasRange)
            {
                // Range for the second argument (value)
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
    juce::String basePath = "/wfs/reverb/" + juce::String(channelIndex);
    auto* channel = makeContainerNode(basePath, "Reverb " + juce::String(channelIndex));
    auto* contents = channel->getProperties()["CONTENTS"].getDynamicObject();

    const auto& addrMap = OSCMessageRouter::getReverbAddressMap();

    for (const auto& [oscName, paramId] : addrMap)
    {
        juce::String fullPath = basePath + "/" + oscName;

        // Pre-EQ band params take bandIndex as first OSC arg
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

    // Config address map keys are full paths like "/wfs/config/stage/width"
    // Group into sub-containers by the part after "/wfs/config/"
    // Use juce::var for ownership to keep ref counts correct
    std::map<juce::String, juce::var> subContainers;

    for (const auto& [fullOscPath, paramId] : addrMap)
    {
        // Extract sub-path: "/wfs/config/stage/width" -> "stage/width"
        juce::String subPath = fullOscPath.fromFirstOccurrenceOf("/wfs/config/", false, true);
        if (subPath.isEmpty())
            continue;

        // Split into group/param: "stage/width" -> group="stage", param="width"
        int slashIdx = subPath.indexOf("/");
        if (slashIdx < 0)
            continue;

        juce::String group = subPath.substring(0, slashIdx);
        juce::String paramName = subPath.substring(slashIdx + 1);

        // Create sub-container if needed
        if (subContainers.find(group) == subContainers.end())
            subContainers[group] = juce::var(makeContainerNode("/wfs/config/" + group, group + " parameters"));

        auto* subObj = subContainers[group].getDynamicObject();
        auto* subContents = subObj->getProperties()["CONTENTS"].getDynamicObject();

        // Config params are global (no channel index)
        juce::var value = state.getParameter(paramId, -1);
        juce::String typeTag = getOSCTypeTag(value);
        auto range = getParamRange(paramId);

        // Post-EQ params take band index as first arg
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

    // Add sub-containers to config
    for (auto& [group, subVar] : subContainers)
        configContents->setProperty(group, subVar);

    return configContainer;
}

//==============================================================================
// ValueTree::Listener
//==============================================================================

void OSCQueryServer::valueTreePropertyChanged(juce::ValueTree& /*tree*/,
                                              const juce::Identifier& /*property*/)
{
    // JSON is regenerated on each request, so no cache invalidation needed.
    // When WebSocket/LISTEN is added, this will push PATH_CHANGED notifications.
}

} // namespace WFSNetwork
