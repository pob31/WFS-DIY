#include "TrackingPSNReceiver.h"

namespace WFSNetwork
{

//==============================================================================
// TrackingPSNReceiver
//==============================================================================

TrackingPSNReceiver::TrackingPSNReceiver(WFSValueTreeState& valueTreeState)
    : Thread("PSN Tracking Receiver")
    , state(valueTreeState)
{
}

TrackingPSNReceiver::~TrackingPSNReceiver()
{
    stop();
}

bool TrackingPSNReceiver::start(int portNumber,
                                 const juce::String& interface,
                                 const juce::String& multicast)
{
    // Stop any existing receiver
    stop();

    port = portNumber;
    networkInterface = interface;
    multicastAddress = multicast;

    // Bind socket to port
    if (!socket.bindToPort(port, networkInterface))
    {
        DBG("TrackingPSNReceiver: Failed to bind to port " << port);
        return false;
    }

    // Join multicast group
    if (!socket.joinMulticast(multicastAddress))
    {
        DBG("TrackingPSNReceiver: Failed to join multicast group " << multicastAddress);
        socket.shutdown();
        return false;
    }

    // Start the receiver thread
    shouldStop = false;
    startThread();

    DBG("TrackingPSNReceiver: Started on port " << port
        << " multicast " << multicastAddress
        << (networkInterface.isNotEmpty() ? " interface " + networkInterface : ""));
    return true;
}

void TrackingPSNReceiver::stop()
{
    if (isThreadRunning())
    {
        shouldStop = true;
        socket.shutdown();  // Unblock any waiting read
        stopThread(1000);   // Wait up to 1 second
        DBG("TrackingPSNReceiver: Stopped");
    }
}

void TrackingPSNReceiver::setTransformations(float newOffsetX, float newOffsetY, float newOffsetZ,
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

TrackingPSNReceiver::Statistics TrackingPSNReceiver::getStatistics() const
{
    return {
        packetsReceived.load(),
        trackersProcessed.load(),
        positionsRouted.load(),
        orientationsRouted.load()
    };
}

void TrackingPSNReceiver::resetStatistics()
{
    packetsReceived = 0;
    trackersProcessed = 0;
    positionsRouted = 0;
    orientationsRouted = 0;
}

//==============================================================================
// Thread
//==============================================================================

void TrackingPSNReceiver::run()
{
    char buffer[::psn::MAX_UDP_PACKET_SIZE];

    while (!shouldStop.load() && !threadShouldExit())
    {
        // Wait for data with timeout to allow checking shouldStop periodically
        if (!socket.waitUntilReady(true, 50))  // 50ms timeout
            continue;

        int bytesRead = socket.read(buffer, sizeof(buffer), false);

        if (bytesRead > 0)
        {
            ++packetsReceived;

            // Decode the PSN packet
            if (decoder.decode(buffer, static_cast<size_t>(bytesRead)))
            {
                // Process all trackers in the decoded data
                const auto& data = decoder.get_data();
                for (const auto& [id, tracker] : data.trackers)
                {
                    processTrackerData(tracker);
                }
            }
        }
    }

    // Leave multicast group on exit
    socket.leaveMulticast(multicastAddress);
}

//==============================================================================
// Internal Processing
//==============================================================================

void TrackingPSNReceiver::processTrackerData(const ::psn::tracker& tracker)
{
    ++trackersProcessed;

    int trackingId = tracker.get_id();

    // Process position if available
    if (tracker.is_pos_set())
    {
        auto pos = tracker.get_pos();

        // Apply transformations: offset -> scale -> flip
        float x = (pos.x + offsetX.load()) * scaleX.load();
        float y = (pos.y + offsetY.load()) * scaleY.load();
        float z = (pos.z + offsetZ.load()) * scaleZ.load();

        if (flipX.load()) x = -x;
        if (flipY.load()) y = -y;
        if (flipZ.load()) z = -z;

        routePositionToInputs(trackingId, x, y, z);
    }

    // Process orientation if available
    if (tracker.is_ori_set())
    {
        auto ori = tracker.get_ori();
        // PSN orientation: x=pitch, y=roll, z=yaw (in degrees typically)
        // Map Z-axis (yaw) to inputRotation for directivity
        float rotation = ori.z;
        routeOrientationToInputs(trackingId, rotation);
    }
}

void TrackingPSNReceiver::routePositionToInputs(int trackingId, float x, float y, float z)
{
    int numInputs = state.getNumInputChannels();
    bool anyRouted = false;

    for (int ch = 0; ch < numInputs; ++ch)
    {
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

        // Update offset coordinates (tracking updates offset, not base position)
        // Using setProperty triggers ValueTree listeners which updates map and broadcasts to targets
        posSection.setProperty(WFSParameterIDs::inputOffsetX, x, nullptr);
        posSection.setProperty(WFSParameterIDs::inputOffsetY, y, nullptr);
        posSection.setProperty(WFSParameterIDs::inputOffsetZ, z, nullptr);

        anyRouted = true;
    }

    if (anyRouted)
        ++positionsRouted;
}

void TrackingPSNReceiver::routeOrientationToInputs(int trackingId, float rotation)
{
    int numInputs = state.getNumInputChannels();
    bool anyRouted = false;

    for (int ch = 0; ch < numInputs; ++ch)
    {
        auto posSection = state.getInputPositionSection(ch);
        auto directivitySection = state.getInputDirectivitySection(ch);

        if (!posSection.isValid() || !directivitySection.isValid())
            continue;

        // Check if this input's tracking ID matches
        int inputTrackingId = posSection.getProperty(WFSParameterIDs::inputTrackingID, 0);
        if (inputTrackingId != trackingId)
            continue;

        // Check if tracking is active for this input
        bool trackingActive = posSection.getProperty(WFSParameterIDs::inputTrackingActive, false);
        if (!trackingActive)
            continue;

        // Update inputRotation in directivity section
        directivitySection.setProperty(WFSParameterIDs::inputRotation, rotation, nullptr);

        anyRouted = true;
    }

    if (anyRouted)
        ++orientationsRouted;
}

} // namespace WFSNetwork
