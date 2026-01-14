#include "TrackingRTTrPReceiver.h"
#include <cmath>

namespace WFSNetwork
{

//==============================================================================
// TrackingRTTrPReceiver
//==============================================================================

TrackingRTTrPReceiver::TrackingRTTrPReceiver(WFSValueTreeState& valueTreeState)
    : Thread("RTTrP Tracking Receiver")
    , state(valueTreeState)
{
}

TrackingRTTrPReceiver::~TrackingRTTrPReceiver()
{
    stop();
}

bool TrackingRTTrPReceiver::start(int portNumber)
{
    // Stop any existing receiver
    stop();

    port = portNumber;

    // Bind socket to port
    if (!socket.bindToPort(port))
    {
        DBG("TrackingRTTrPReceiver: Failed to bind to port " << port);
        return false;
    }

    // Start the receiver thread
    shouldStop = false;
    startThread();

    DBG("TrackingRTTrPReceiver: Started on port " << port);
    return true;
}

void TrackingRTTrPReceiver::stop()
{
    if (isThreadRunning())
    {
        shouldStop = true;
        socket.shutdown();  // Unblock any waiting read
        stopThread(1000);   // Wait up to 1 second
        DBG("TrackingRTTrPReceiver: Stopped");
    }
}

void TrackingRTTrPReceiver::setTransformations(float newOffsetX, float newOffsetY, float newOffsetZ,
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

TrackingRTTrPReceiver::Statistics TrackingRTTrPReceiver::getStatistics() const
{
    return {
        packetsReceived.load(),
        trackersProcessed.load(),
        positionsRouted.load(),
        orientationsRouted.load()
    };
}

void TrackingRTTrPReceiver::resetStatistics()
{
    packetsReceived = 0;
    trackersProcessed = 0;
    positionsRouted = 0;
    orientationsRouted = 0;
}

//==============================================================================
// Thread
//==============================================================================

void TrackingRTTrPReceiver::run()
{
    char buffer[RTTrP::MAX_PACKET_SIZE];

    while (!shouldStop.load() && !threadShouldExit())
    {
        // Wait for data with timeout to allow checking shouldStop periodically
        if (!socket.waitUntilReady(true, 50))  // 50ms timeout
            continue;

        int bytesRead = socket.read(buffer, sizeof(buffer), false);

        if (bytesRead > 0)
        {
            ++packetsReceived;

            // Decode the RTTrP packet
            if (decoder.decode(buffer, static_cast<size_t>(bytesRead)))
            {
                // Process all trackables in the decoded data
                for (const auto& [id, trackable] : decoder.getTrackables())
                {
                    processTrackable(trackable);
                }
            }
        }
    }
}

//==============================================================================
// Internal Processing
//==============================================================================

void TrackingRTTrPReceiver::processTrackable(const RTTrP::Trackable& trackable)
{
    ++trackersProcessed;

    int trackingId = trackable.id;

    // Process position if available
    if (trackable.hasPosition)
    {
        const auto& pos = trackable.position;

        // Apply transformations: offset -> scale -> flip
        float x = static_cast<float>((pos.x + offsetX.load()) * scaleX.load());
        float y = static_cast<float>((pos.y + offsetY.load()) * scaleY.load());
        float z = static_cast<float>((pos.z + offsetZ.load()) * scaleZ.load());

        if (flipX.load()) x = -x;
        if (flipY.load()) y = -y;
        if (flipZ.load()) z = -z;

        routePositionToInputs(trackingId, x, y, z);
    }

    // Process orientation if available
    // Prefer quaternion if available, otherwise use Euler angles
    if (trackable.hasQuaternion)
    {
        float rotation = quaternionToYaw(trackable.quaternion);
        routeOrientationToInputs(trackingId, rotation);
    }
    else if (trackable.hasEuler)
    {
        // Use R3 as yaw (depends on rotation order, but commonly the last rotation is yaw)
        float rotation = static_cast<float>(trackable.euler.r3);
        routeOrientationToInputs(trackingId, rotation);
    }
}

float TrackingRTTrPReceiver::quaternionToYaw(const RTTrP::Quaternion& q) const
{
    // Convert quaternion to yaw (rotation around Z axis)
    // Formula: yaw = atan2(2*(qw*qz + qx*qy), 1 - 2*(qy*qy + qz*qz))
    double siny_cosp = 2.0 * (q.qw * q.qz + q.qx * q.qy);
    double cosy_cosp = 1.0 - 2.0 * (q.qy * q.qy + q.qz * q.qz);
    double yaw = std::atan2(siny_cosp, cosy_cosp);

    // Convert radians to degrees
    return static_cast<float>(yaw * 180.0 / juce::MathConstants<double>::pi);
}

void TrackingRTTrPReceiver::routePositionToInputs(int trackingId, float x, float y, float z)
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

void TrackingRTTrPReceiver::routeOrientationToInputs(int trackingId, float rotation)
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
