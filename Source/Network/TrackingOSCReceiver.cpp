#include "TrackingOSCReceiver.h"

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

    // Create and start the receiver
    receiver = std::make_unique<OSCReceiverWithSenderIP>();
    receiver->addListener(this);

    if (!receiver->connect(port))
    {
        DBG("TrackingOSCReceiver: Failed to bind to port " << port);
        receiver.reset();
        return false;
    }

    DBG("TrackingOSCReceiver: Started on port " << port << " with pattern: " << pathPattern);
    return true;
}

void TrackingOSCReceiver::stop()
{
    if (receiver)
    {
        receiver->removeListener(this);
        receiver->disconnect();
        receiver.reset();
        DBG("TrackingOSCReceiver: Stopped");
    }
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
    bool hasX, hasY, hasZ;
    float x = localPattern.extractX(message, hasX);
    float y = localPattern.extractY(message, hasY);
    float z = localPattern.extractZ(message, hasZ);

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

    // Route to matching inputs
    routeToInputs(trackingId, x, y, z, hasX, hasY, hasZ);
}

void TrackingOSCReceiver::routeToInputs(int trackingId, float x, float y, float z,
                                         bool hasX, bool hasY, bool hasZ)
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

        // Update offset coordinates (tracking updates offset, not position)
        // Using setProperty triggers ValueTree listeners which updates map and broadcasts to targets
        if (hasX)
            posSection.setProperty(WFSParameterIDs::inputOffsetX, x, nullptr);
        if (hasY)
            posSection.setProperty(WFSParameterIDs::inputOffsetY, y, nullptr);
        if (hasZ)
            posSection.setProperty(WFSParameterIDs::inputOffsetZ, z, nullptr);

        anyRouted = true;
    }

    if (anyRouted)
        ++messagesRouted;
}

} // namespace WFSNetwork
