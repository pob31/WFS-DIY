#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"

// PSN library - header-only implementation
// Windows defines min/max macros that conflict with std::min/max used in PSN library
#ifdef _WIN32
  #ifdef min
    #undef min
  #endif
  #ifdef max
    #undef max
  #endif
#endif
#include "../../ThirdParty/PSN-CPP/psn_lib.hpp"

namespace WFSNetwork
{

/**
 * TrackingPSNReceiver
 *
 * Dedicated PSN (PosiStageNet) receiver for tracking data.
 * Listens on UDP multicast for PSN packets,
 * applies transformations (offset, scale, flip),
 * and routes position/orientation to inputs with matching tracking IDs.
 *
 * PSN Protocol:
 * - Default port: 56565
 * - Multicast address: 236.10.10.10
 * - Provides position, speed, acceleration, orientation, and status per tracker
 */
class TrackingPSNReceiver : private juce::Thread
{
public:
    /**
     * Constructor.
     * @param valueTreeState Reference to parameter state for routing to inputs
     */
    explicit TrackingPSNReceiver(WFSValueTreeState& valueTreeState);
    ~TrackingPSNReceiver() override;

    /**
     * Start listening for PSN packets.
     * @param port UDP port to listen on (default: 56565)
     * @param networkInterface Network interface to bind to (empty = default)
     * @param multicastAddress PSN multicast group (default: 236.10.10.10)
     * @return true if started successfully
     */
    bool start(int port = 56565,
               const juce::String& networkInterface = "",
               const juce::String& multicastAddress = "236.10.10.10");

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

    // Process tracker data from decoded PSN packet
    void processTrackerData(const ::psn::tracker& tracker);

    // Route position to inputs with matching tracking ID
    void routePositionToInputs(int trackingId, float x, float y, float z);

    // Route orientation to inputs (updates inputRotation in directivity)
    void routeOrientationToInputs(int trackingId, float rotation);

    WFSValueTreeState& state;

    // Socket for UDP reception
    juce::DatagramSocket socket;

    // PSN decoder
    ::psn::psn_decoder decoder;

    // Configuration
    int port = 56565;
    juce::String multicastAddress = "236.10.10.10";
    juce::String networkInterface;

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackingPSNReceiver)
};

} // namespace WFSNetwork
