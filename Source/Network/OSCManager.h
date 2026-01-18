#pragma once

#include <JuceHeader.h>
#include "OSCProtocolTypes.h"
#include "OSCConnection.h"
#include "OSCRateLimiter.h"
#include "OSCLogger.h"
#include "OSCMessageBuilder.h"
#include "OSCMessageRouter.h"
#include "OSCReceiverWithSenderIP.h"
#include "OSCTCPReceiver.h"
#include "OSCQueryServer.h"
#include "TrackingOSCReceiver.h"
#include "TrackingPSNReceiver.h"
#include "TrackingRTTrPReceiver.h"
#include "../Parameters/WFSValueTreeState.h"

namespace WFSNetwork
{

/**
 * OSCManager
 *
 * Central coordinator for all OSC communication.
 * Manages bidirectional OSC for up to 6 targets with rate limiting.
 * Supports IP filtering for incoming messages (UDP and TCP).
 */
class OSCManager : public juce::ValueTree::Listener,
                   public juce::Timer
{
public:
    //==========================================================================
    // Construction / Destruction
    //==========================================================================

    OSCManager(WFSValueTreeState& valueTreeState);
    ~OSCManager() override;

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * Apply global network configuration.
     * Called when network settings change in UI.
     */
    void applyGlobalConfig(const GlobalConfig& config);

    /**
     * Apply configuration for a specific target.
     */
    void applyTargetConfig(int targetIndex, const TargetConfig& config);

    /**
     * Get current target configuration.
     */
    TargetConfig getTargetConfig(int targetIndex) const;

    /**
     * Get current global configuration.
     */
    GlobalConfig getGlobalConfig() const;

    //==========================================================================
    // Connection Control
    //==========================================================================

    /**
     * Start listening on configured ports.
     */
    bool startListening();

    /**
     * Stop listening.
     */
    void stopListening();

    /**
     * Check if listening is active.
     */
    bool isListening() const { return listening; }

    /**
     * Connect a specific target.
     */
    bool connectTarget(int targetIndex);

    /**
     * Disconnect a specific target.
     */
    void disconnectTarget(int targetIndex);

    /**
     * Disconnect all targets.
     */
    void disconnectAll();

    /**
     * Get connection status for a target.
     */
    ConnectionStatus getTargetStatus(int targetIndex) const;

    //==========================================================================
    // Message Sending (Manual)
    //==========================================================================

    /**
     * Send an OSC message to a specific target.
     * Goes through rate limiter.
     */
    void sendMessage(int targetIndex, const juce::OSCMessage& message);

    /**
     * Send an OSC message to all connected targets.
     */
    void broadcastMessage(const juce::OSCMessage& message);

    /**
     * Send all queued messages immediately (bypasses rate limiter).
     */
    void flushMessages();

    //==========================================================================
    // REMOTE Protocol
    //==========================================================================

    /**
     * Set the currently selected input channel for REMOTE protocol.
     * When set, parameter changes for this channel are sent at 50Hz max.
     */
    void setRemoteSelectedChannel(int channelId);

    /**
     * Get the currently selected REMOTE channel.
     */
    int getRemoteSelectedChannel() const { return remoteSelectedChannel; }

    /**
     * Send /findDevice command to all connected REMOTE targets.
     * Makes the remote device flash/buzz to help locate it.
     * @param password The password string (can be empty)
     */
    void sendFindDevice(const juce::String& password);

    //==========================================================================
    // IP Filtering
    //==========================================================================

    /**
     * Enable/disable IP filtering.
     * When enabled, only messages from target IPs are processed.
     */
    void setIPFilteringEnabled(bool enabled);
    bool isIPFilteringEnabled() const { return ipFilteringEnabled; }

    //==========================================================================
    // OSC Query
    //==========================================================================

    /**
     * Start the OSC Query server for parameter discovery.
     * @param oscPort UDP port for OSC messages (typically same as UDP receive port)
     * @param httpPort HTTP port for OSC Query discovery (e.g., 5005)
     * @return true if server started successfully
     */
    bool startOSCQuery(int oscPort, int httpPort);

    /**
     * Stop the OSC Query server.
     */
    void stopOSCQuery();

    /**
     * Check if OSC Query server is running.
     */
    bool isOSCQueryRunning() const;

    //==========================================================================
    // Tracking OSC
    //==========================================================================

    /**
     * Start the tracking OSC receiver.
     * @param port UDP port to listen on for tracking data
     * @param pathPattern OSC path pattern (e.g., "/wfs/tracking <ID> <x> <y> <z>")
     * @return true if started successfully
     */
    bool startTrackingReceiver(int port, const juce::String& pathPattern);

    /**
     * Stop the tracking OSC receiver.
     */
    void stopTrackingReceiver();

    /**
     * Check if tracking OSC receiver is running.
     */
    bool isTrackingReceiverRunning() const;

    /**
     * Update tracking transformations (offset, scale, flip).
     */
    void updateTrackingTransformations(float offsetX, float offsetY, float offsetZ,
                                        float scaleX, float scaleY, float scaleZ,
                                        bool flipX, bool flipY, bool flipZ);

    /**
     * Update tracking path pattern while receiver is running.
     * @return true if pattern is valid
     */
    bool updateTrackingPathPattern(const juce::String& pathPattern);

    //==========================================================================
    // PSN Tracking
    //==========================================================================

    /**
     * Start the PSN tracking receiver.
     * @param port UDP port to listen on (default 56565)
     * @param networkInterface Network interface for multicast (empty = default)
     * @param multicastAddress PSN multicast group (default 236.10.10.10)
     * @return true if started successfully
     */
    bool startPSNReceiver(int port = 56565,
                          const juce::String& networkInterface = "",
                          const juce::String& multicastAddress = "236.10.10.10");

    /**
     * Stop the PSN tracking receiver.
     */
    void stopPSNReceiver();

    /**
     * Check if PSN tracking receiver is running.
     */
    bool isPSNReceiverRunning() const;

    /**
     * Update PSN tracking transformations (offset, scale, flip).
     */
    void updatePSNTransformations(float offsetX, float offsetY, float offsetZ,
                                   float scaleX, float scaleY, float scaleZ,
                                   bool flipX, bool flipY, bool flipZ);

    //==========================================================================
    // RTTrP Tracking
    //==========================================================================

    /**
     * Start the RTTrP tracking receiver.
     * @param port UDP port to listen on (default 24220)
     * @return true if started successfully
     */
    bool startRTTrPReceiver(int port = RTTrP::DEFAULT_PORT);

    /**
     * Stop the RTTrP tracking receiver.
     */
    void stopRTTrPReceiver();

    /**
     * Check if RTTrP tracking receiver is running.
     */
    bool isRTTrPReceiverRunning() const;

    /**
     * Update RTTrP tracking transformations (offset, scale, flip).
     */
    void updateRTTrPTransformations(float offsetX, float offsetY, float offsetZ,
                                     float scaleX, float scaleY, float scaleZ,
                                     bool flipX, bool flipY, bool flipZ);

    //==========================================================================
    // Logging
    //==========================================================================

    /**
     * Get the OSC logger for UI display.
     */
    OSCLogger& getLogger() { return logger; }

    /**
     * Enable/disable logging.
     */
    void setLoggingEnabled(bool enabled);

    //==========================================================================
    // Statistics
    //==========================================================================

    struct Statistics
    {
        int messagesSent = 0;
        int messagesReceived = 0;
        int messagesCoalesced = 0;
        int parseErrors = 0;
    };

    Statistics getStatistics() const;
    void resetStatistics();

    //==========================================================================
    // Callbacks
    //==========================================================================

    /**
     * Callback when a REMOTE inputNumber message is received.
     * UI can register to switch channel selection.
     */
    std::function<void(int channelId)> onRemoteChannelSelect;

    /**
     * Callback when connection status changes for any target.
     */
    std::function<void(int targetIndex, ConnectionStatus status)> onConnectionStatusChanged;

    /**
     * Callback when position data is received from REMOTE protocol.
     * UI can register to trigger map repaint.
     */
    std::function<void()> onRemotePositionReceived;

    /**
     * Callback when position data is received via OSC or REMOTE protocol.
     * Used for path mode waypoint capture when max speed + path mode are enabled.
     * @param channelIndex 0-based channel index
     * @param x, y, z The new position coordinates (after constraints applied)
     */
    std::function<void(int channelIndex, float x, float y, float z)> onRemoteWaypointCapture;

private:
    //==========================================================================
    // ValueTree::Listener
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override {}
    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree&) override {}

    //==========================================================================
    // OSC Receiver Listeners (nested classes to track transport type)
    //==========================================================================

    /** Internal listener for UDP receiver */
    class UDPListener : public OSCReceiverWithSenderIP::Listener
    {
    public:
        explicit UDPListener(OSCManager& owner) : owner(owner) {}
        void oscMessageReceived(const juce::OSCMessage& message, const juce::String& senderIP) override;
        void oscBundleReceived(const juce::OSCBundle& bundle, const juce::String& senderIP) override;
    private:
        OSCManager& owner;
    };

    /** Internal listener for TCP receiver */
    class TCPListener : public OSCReceiverWithSenderIP::Listener
    {
    public:
        explicit TCPListener(OSCManager& owner) : owner(owner) {}
        void oscMessageReceived(const juce::OSCMessage& message, const juce::String& senderIP) override;
        void oscBundleReceived(const juce::OSCBundle& bundle, const juce::String& senderIP) override;
    private:
        OSCManager& owner;
    };

    friend class UDPListener;
    friend class TCPListener;

    //==========================================================================
    // Timer
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // Internal Methods
    //==========================================================================

    void handleIncomingMessage(const juce::OSCMessage& message,
                               const juce::String& senderIP,
                               int port,
                               ConnectionMode transport);
    void handleIncomingBundle(const juce::OSCBundle& bundle,
                              const juce::String& senderIP,
                              int port,
                              ConnectionMode transport);
    void handleStandardOSCMessage(const juce::OSCMessage& message);
    void handleRemoteInputMessage(const juce::OSCMessage& message);
    void handleRemotePositionDelta(const OSCMessageRouter::ParsedRemoteInput& parsed);
    void handleRemoteParameterSet(const OSCMessageRouter::ParsedRemoteInput& parsed);
    void handleRemoteParameterDelta(const OSCMessageRouter::ParsedRemoteInput& parsed);
    void handleArrayAdjustMessage(const juce::OSCMessage& message);
    void handleClusterMoveMessage(const juce::OSCMessage& message);

    // Stage bounds for constraint application
    float getStageMinX() const;
    float getStageMaxX() const;
    float getStageMinY() const;
    float getStageMaxY() const;
    float getStageMaxZ() const;

    // Apply constraints to position values (returns constrained value)
    float applyConstraintX(int channelIndex, float value) const;
    float applyConstraintY(int channelIndex, float value) const;
    float applyConstraintZ(int channelIndex, float value) const;
    // Apply distance constraint for Cylindrical/Spherical modes (modifies position in place)
    void applyConstraintDistance(int channelIndex, float& x, float& y, float& z) const;

    void sendRemoteChannelDump(int channelId);

    /**
     * Send stage configuration to all connected Remote targets.
     * Sends origin, dimensions, shape, and input count.
     */
    void sendStageConfigToRemote();

    void sendParameterUpdate(int targetIndex, const juce::Identifier& paramId,
                             int channelId, float value, bool isOutput);

    bool isAllowedIP(const juce::String& senderIP) const;

    void updateTargetStatus(int targetIndex, ConnectionStatus newStatus);

    //==========================================================================
    // Members
    //==========================================================================

    WFSValueTreeState& state;

    // Receivers (custom implementations that expose sender IP)
    std::unique_ptr<OSCReceiverWithSenderIP> udpReceiver;
    std::unique_ptr<OSCTCPReceiver> tcpReceiver;
    UDPListener udpListener { *this };
    TCPListener tcpListener { *this };
    bool listening = false;

    // Connections (one per target)
    std::array<std::unique_ptr<OSCConnection>, MAX_TARGETS> connections;
    std::array<TargetConfig, MAX_TARGETS> targetConfigs;
    std::array<ConnectionStatus, MAX_TARGETS> targetStatuses;

    // Global config
    GlobalConfig globalConfig;

    // Rate limiting
    OSCRateLimiter rateLimiter;

    // Logging
    OSCLogger logger;

    // IP filtering
    bool ipFilteringEnabled = false;

    // OSC Query server
    std::unique_ptr<OSCQueryServer> oscQueryServer;

    // Tracking OSC receiver
    std::unique_ptr<TrackingOSCReceiver> trackingReceiver;

    // Tracking PSN receiver
    std::unique_ptr<TrackingPSNReceiver> psnReceiver;

    // Tracking RTTrP receiver
    std::unique_ptr<TrackingRTTrPReceiver> rttrpReceiver;

    // REMOTE protocol state
    int remoteSelectedChannel = 1;
    std::set<juce::Identifier> remoteModifiedParams;

    // Loop prevention: tracks the protocol type of incoming message being processed
    // Set to Protocol::Disabled when not processing an incoming message
    // When set, only blocks re-sending to targets of the SAME protocol type
    Protocol incomingProtocol = Protocol::Disabled;

    // Statistics
    std::atomic<int> messagesSent { 0 };
    std::atomic<int> messagesReceived { 0 };
    std::atomic<int> parseErrors { 0 };

    // Thread safety
    juce::CriticalSection configLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSCManager)
};

} // namespace WFSNetwork
