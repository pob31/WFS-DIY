#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"
#include "RTTrPDecoder.h"

namespace WFSNetwork
{

/**
 * TrackingRTTrPReceiver
 *
 * Dedicated RTTrP (Real-Time Tracking Protocol) receiver for motion tracking data.
 * Listens on UDP for RTTrPM packets,
 * applies transformations (offset, scale, flip),
 * and routes position/orientation to inputs with matching tracking IDs.
 *
 * RTTrP Protocol:
 * - Default port: 24220
 * - Transport: UDP (unicast or multicast)
 * - Provides position, quaternion/euler orientation, velocity, acceleration per trackable
 *
 * Cross-platform: Uses JUCE DatagramSocket and standard C++ only.
 */
class TrackingRTTrPReceiver : private juce::Thread
{
public:
    /**
     * Constructor.
     * @param valueTreeState Reference to parameter state for routing to inputs
     */
    explicit TrackingRTTrPReceiver(WFSValueTreeState& valueTreeState);
    ~TrackingRTTrPReceiver() override;

    /**
     * Start listening for RTTrP packets.
     * @param port UDP port to listen on (default: 24220)
     * @return true if started successfully
     */
    bool start(int port = RTTrP::DEFAULT_PORT);

    /**
     * Stop listening.
     */
    void stop();

    /**
     * Check if receiver is active.
     */
    bool isActive() const { return isThreadRunning(); }

    /**
     * Update transformation parameters.
     * Called when offset/scale/flip values change.
     */
    void setTransformations(float offsetX, float offsetY, float offsetZ,
                            float scaleX, float scaleY, float scaleZ,
                            bool flipX, bool flipY, bool flipZ);

    /**
     * Get statistics.
     */
    struct Statistics
    {
        int packetsReceived = 0;
        int trackersProcessed = 0;
        int positionsRouted = 0;
        int orientationsRouted = 0;
    };
    Statistics getStatistics() const;
    void resetStatistics();

private:
    // Thread run loop
    void run() override;

    // Process trackable data from decoded RTTrP packet
    void processTrackable(const RTTrP::Trackable& trackable);

    // Route position to inputs with matching tracking ID
    void routePositionToInputs(int trackingId, float x, float y, float z);

    // Route orientation to inputs (updates inputRotation in directivity)
    void routeOrientationToInputs(int trackingId, float rotation);

    // Convert quaternion to yaw angle (rotation around Z axis)
    float quaternionToYaw(const RTTrP::Quaternion& q) const;

    WFSValueTreeState& state;

    // Socket for UDP reception
    juce::DatagramSocket socket;

    // RTTrP decoder
    RTTrP::Decoder decoder;

    // Configuration
    int port = RTTrP::DEFAULT_PORT;

    // Thread control
    std::atomic<bool> shouldStop { false };

    // Transformation parameters
    std::atomic<float> offsetX { 0.0f };
    std::atomic<float> offsetY { 0.0f };
    std::atomic<float> offsetZ { 0.0f };
    std::atomic<float> scaleX { 1.0f };
    std::atomic<float> scaleY { 1.0f };
    std::atomic<float> scaleZ { 1.0f };
    std::atomic<bool> flipX { false };
    std::atomic<bool> flipY { false };
    std::atomic<bool> flipZ { false };

    // Statistics
    std::atomic<int> packetsReceived { 0 };
    std::atomic<int> trackersProcessed { 0 };
    std::atomic<int> positionsRouted { 0 };
    std::atomic<int> orientationsRouted { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackingRTTrPReceiver)
};

} // namespace WFSNetwork
