#include "TrackingOSCReceiver.h"
#include "../../spatcore/dsp/TrackingPositionFilter.h"
#include "../../spatcore/control/osc/OSCParser.h"
#include "../WFSLogger.h"
#include "OSCLogger.h"

namespace WFSNetwork
{

//==============================================================================
// TrackingOSCReceiver
//==============================================================================

TrackingOSCReceiver::TrackingOSCReceiver(WFSValueTreeState& valueTreeState)
    : state(valueTreeState)
{
}

TrackingOSCReceiver::~TrackingOSCReceiver()
{
    stop();
}

bool TrackingOSCReceiver::start(int port, const juce::String& pathPattern)
{
    // Stop any existing receiver
    stop();

    // Parse the path pattern
    {
        juce::ScopedLock sl(patternLock);
        if (!pattern.parse(pathPattern))
        {
            DBG("TrackingOSCReceiver: Invalid path pattern: " << pathPattern);
            return false;
        }
    }

    // Every datagram goes through a bounded queue drained on the message
    // thread, as the main OSC receivers do (OSCManager). Without a raw-data
    // callback the receiver posted one callAsync per datagram holding its raw
    // `this`: a flood grew the message queue without limit, and a stop() (port
    // change, tracking switched off, shutdown) freed the receiver under the
    // calls still queued (re-audit 2026-09-29, N5). Nothing coalesces: the
    // address is the user's pattern and may carry every tracker at once.
    using spatcore::control::osc::OSCIngestQueue;
    ingestQueue = std::make_unique<OSCIngestQueue>(OSCIngestQueue::Classifier {});
    ingestQueue->setMaxItemsPerTick(256);   // the whole FIFO per tick: the cap bounds memory, not throughput
    ingestQueue->setDrainIntervalMs(5);
    ingestQueue->setDispatch([this] (const juce::MemoryBlock& data, const juce::String& senderIP,
                                     int, spatcore::control::osc::ConnectionMode)
    {
        dispatchIngested(data, senderIP);
    });
    ingestQueue->setDropReport([] (uint64_t totalDropped, spatcore::control::osc::ConnectionMode)
    {
        WFSLogger::getInstance().logWarning("Tracking OSC queue full, dropped "
                                            + juce::String(totalDropped) + " message(s) in total");
    });

    // Create and start the receiver
    receiver = std::make_unique<OSCReceiverWithSenderIP>();
    auto* queuePtr = ingestQueue.get();
    receiver->setRawDataCallback([queuePtr, port] (juce::MemoryBlock data, juce::String senderIP, int)
    {
        queuePtr->push(std::move(data), std::move(senderIP), port, spatcore::control::osc::ConnectionMode::UDP);
    });

    if (!receiver->connect(port))
    {
        DBG("TrackingOSCReceiver: Failed to bind to port " << port);
        receiver.reset();
        ingestQueue.reset();
        return false;
    }

    return true;
}

void TrackingOSCReceiver::stop()
{
    // In this order: disconnect joins the socket thread, so nothing pushes
    // into the queue after it; the queue goes next, on the message thread,
    // and its drain timer with it.
    if (receiver)
    {
        receiver->disconnect();
        receiver.reset();
    }
    ingestQueue.reset();
}

void TrackingOSCReceiver::setTransformations(float newOffsetX, float newOffsetY, float newOffsetZ,
                                              float newScaleX, float newScaleY, float newScaleZ,
                                              bool newFlipX, bool newFlipY, bool newFlipZ)
{
    offsetX = newOffsetX;
    offsetY = newOffsetY;
    offsetZ = newOffsetZ;
    scaleX = newScaleX;
    scaleY = newScaleY;
    scaleZ = newScaleZ;
    flipX = newFlipX;
    flipY = newFlipY;
    flipZ = newFlipZ;
}

bool TrackingOSCReceiver::setPathPattern(const juce::String& pathPattern)
{
    juce::ScopedLock sl(patternLock);
    return pattern.parse(pathPattern);
}

TrackingOSCReceiver::Statistics TrackingOSCReceiver::getStatistics() const
{
    return { messagesReceived.load(), messagesMatched.load(), messagesRouted.load() };
}

void TrackingOSCReceiver::resetStatistics()
{
    messagesReceived = 0;
    messagesMatched = 0;
    messagesRouted = 0;
}

//==============================================================================
// OSCReceiverWithSenderIP::Listener
//==============================================================================

void TrackingOSCReceiver::dispatchIngested(const juce::MemoryBlock& data, const juce::String& senderIP)
{
    try
    {
        const char* bytes = static_cast<const char*>(data.getData());
        const int size = static_cast<int>(data.getSize());
        int pos = 0;

        if (size >= 8 && std::memcmp(bytes, "#bundle", 7) == 0)
            oscBundleReceived(spatcore::control::osc::OSCParser::parseBundle(bytes, size, pos), senderIP);
        else
            oscMessageReceived(spatcore::control::osc::OSCParser::parseMessage(bytes, size, pos), senderIP);
    }
    catch (const juce::OSCFormatError&)
    {
        DBG("TrackingOSCReceiver: parse error from " << senderIP);
    }
}

void TrackingOSCReceiver::oscMessageReceived(const juce::OSCMessage& message,
                                              const juce::String& /*senderIP*/)
{
    ++messagesReceived;
    processTrackingMessage(message);
}

void TrackingOSCReceiver::oscBundleReceived(const juce::OSCBundle& bundle,
                                             const juce::String& senderIP)
{
    // Process each element in the bundle
    for (const auto& element : bundle)
    {
        if (element.isMessage())
        {
            oscMessageReceived(element.getMessage(), senderIP);
        }
        else if (element.isBundle())
        {
            oscBundleReceived(element.getBundle(), senderIP);
        }
    }
}

//==============================================================================
// Internal Processing
//==============================================================================

void TrackingOSCReceiver::processTrackingMessage(const juce::OSCMessage& message)
{
    // Check if message matches our pattern
    TrackingPathPattern localPattern;
    {
        juce::ScopedLock sl(patternLock);
        localPattern = pattern;
    }

    if (!localPattern.matches(message))
        return;

    ++messagesMatched;

    // Extract tracking ID (required)
    int trackingId = localPattern.extractID(message);
    if (trackingId < 1)
    {
        DBG("TrackingOSCReceiver: Invalid tracking ID in message");
        return;
    }

    // Extract coordinates (optional - keep previous if not present)
    bool hasX, hasY, hasZ, hasQ;
    float x = localPattern.extractX(message, hasX);
    float y = localPattern.extractY(message, hasY);
    float z = localPattern.extractZ(message, hasZ);
    float q = localPattern.extractQ(message, hasQ);

    // Apply transformations: offset -> scale -> flip
    if (hasX)
    {
        x += offsetX;
        x *= scaleX;
        if (flipX) x = -x;
    }
    if (hasY)
    {
        y += offsetY;
        y *= scaleY;
        if (flipY) y = -y;
    }
    if (hasZ)
    {
        z += offsetZ;
        z *= scaleZ;
        if (flipZ) z = -z;
    }

    // A sample that is not a number is dropped whole: one NaN poisoned the
    // input's position filter for good and reached its offset. Checked after
    // the transforms, which can take a huge finite value to infinity. (PSN,
    // RTTrP and MQTT get the same check in TrackingIngestQueue::push.)
    if ((hasX && ! std::isfinite (x)) || (hasY && ! std::isfinite (y))
        || (hasZ && ! std::isfinite (z)) || (hasQ && ! std::isfinite (q)))
        return;

    // Route to matching inputs
    routeToInputs(trackingId, x, y, z, hasX, hasY, hasZ, q);
}

void TrackingOSCReceiver::routeToInputs(int trackingId, float x, float y, float z,
                                         bool hasX, bool hasY, bool hasZ,
                                         float qualityFactor)
{
    // Get the number of input channels
    int numInputs = state.getNumInputChannels();
    bool anyRouted = false;

    // Check each input channel
    for (int ch = 0; ch < numInputs; ++ch)
    {
        // Get input's position section directly
        auto posSection = state.getInputPositionSection(ch);
        if (!posSection.isValid())
            continue;

        // Check if this input's tracking ID matches
        int inputTrackingId = posSection.getProperty(WFSParameterIDs::inputTrackingID, 0);
        if (inputTrackingId != trackingId)
            continue;

        // Check if tracking is active for this input
        bool trackingActive = posSection.getProperty(WFSParameterIDs::inputTrackingActive, false);
        if (!trackingActive)
            continue;

        // Apply position filter (smoothing + jump detection)
        float fx = x, fy = y, fz = z;
        float smooth = static_cast<float> (posSection.getProperty (WFSParameterIDs::inputTrackingSmooth, 100));

        if (smooth > 0.0f && positionFilter != nullptr)
        {
            if (! positionFilter->filterPosition (ch, trackingId, fx, fy, fz,
                                                  hasX, hasY, hasZ, smooth, qualityFactor))
                continue; // sample rejected (jump detected)
        }

        // Update offset coordinates (tracking updates offset, not position)
        // The write fires the ValueTree listeners, which update the map and broadcast to targets.
        // Live tracking is transient — suppress dirty flagging in the snapshot scope.
        // Phase 5b: tag as Tracking-origin for the MCP staleness/notifications path.
        OriginTagScope originScope { OriginTag::Tracking };
        ParameterDirtyTracker::ScopedInternalWrite guard (dirtyTracker);
        // Through the store (no undo), so its range clamp and its NaN refusal
        // hold for tracking as for every other writer.
        if (hasX)
            state.setParameterWithoutUndo (WFSParameterIDs::inputOffsetX, fx, ch);
        if (hasY)
            state.setParameterWithoutUndo (WFSParameterIDs::inputOffsetY, fy, ch);
        if (hasZ)
            state.setParameterWithoutUndo (WFSParameterIDs::inputOffsetZ, fz, ch);

        anyRouted = true;
    }

    if (anyRouted)
    {
        ++messagesRouted;

        // Log to Network Log Window
        if (logger != nullptr && logger->getEnabled())
        {
            LogEntry entry;
            entry.timestamp = juce::Time::getCurrentTime();
            entry.direction = "Rx";
            entry.protocol = Protocol::OSC;
            entry.transport = ConnectionMode::UDP;
            entry.address = "/tracking/osc/" + juce::String (trackingId);
            juce::String args;
            if (hasX) args += "x=" + juce::String (x, 3);
            if (hasY) args += (args.isNotEmpty() ? " " : "") + juce::String ("y=") + juce::String (y, 3);
            if (hasZ) args += (args.isNotEmpty() ? " " : "") + juce::String ("z=") + juce::String (z, 3);
            entry.arguments = args;
            logger->logEntry (entry);
        }
    }
}

} // namespace WFSNetwork
