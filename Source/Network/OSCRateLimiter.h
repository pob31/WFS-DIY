#pragma once

#include <JuceHeader.h>
#include "OSCProtocolTypes.h"

namespace WFSNetwork
{

/**
 * OSCRateLimiter - Enforces rate limiting for outgoing OSC messages.
 *
 * Limits messages to MAX_RATE_HZ (50Hz) per target. When rapid parameter changes
 * occur, it coalesces messages using a key of "address:channelId" (for messages with
 * an integer first argument) or just "address" (for other messages).
 *
 * This means:
 * - Messages for different channels are NOT coalesced (each channel keeps its latest value)
 * - Interleaved X/Y/Z updates for the same channel are kept separate but rate-limited together
 * - Only the most recent value per key is sent at each flush interval
 *
 * This prevents network flooding while ensuring timely delivery of parameter updates.
 */
class OSCRateLimiter : private juce::Timer
{
public:
    //==========================================================================
    // Types
    //==========================================================================

    /** Callback when a message should be sent */
    using SendCallback = std::function<void(int targetIndex, const juce::OSCMessage&)>;

    //==========================================================================
    // Construction/Destruction
    //==========================================================================

    explicit OSCRateLimiter(int maxRateHz = MAX_RATE_HZ);
    ~OSCRateLimiter() override;

    //==========================================================================
    // Configuration
    //==========================================================================

    /** Set the maximum send rate in Hz */
    void setMaxRate(int hz);

    /** Get the current max rate */
    int getMaxRate() const { return maxRateHz; }

    /** Get the minimum interval in milliseconds */
    int getMinIntervalMs() const { return minIntervalMs; }

    //==========================================================================
    // Message Queueing
    //==========================================================================

    /**
     * Queue a message for rate-limited sending.
     * If a message with the same address is already queued for this target,
     * it will be replaced (coalesced) with the new one.
     *
     * @param targetIndex Target to send to (0-5), or -1 for broadcast to all enabled
     * @param message The OSC message to send
     */
    void queueMessage(int targetIndex, const juce::OSCMessage& message);

    /**
     * Queue a message to be broadcast to all targets.
     */
    void queueBroadcast(const juce::OSCMessage& message);

    /**
     * Force immediate send of all queued messages (bypassing rate limit).
     * Use sparingly - mainly for shutdown or testing.
     */
    void flushAll();

    /** Clear all pending messages */
    void clearAll();

    //==========================================================================
    // Callbacks
    //==========================================================================

    /** Set the callback for when messages should be sent */
    void setSendCallback(SendCallback callback) { onSend = std::move(callback); }

    //==========================================================================
    // Statistics
    //==========================================================================

    /** Get number of messages queued */
    int getPendingCount() const;

    /** Get number of messages sent (total) */
    int64_t getTotalSent() const { return totalSent; }

    /** Get number of messages coalesced (dropped due to newer value) */
    int64_t getTotalCoalesced() const { return totalCoalesced; }

    /** Reset statistics */
    void resetStats();

private:
    //==========================================================================
    // Timer Override
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // Private Types
    //==========================================================================

    /** Pending messages for a single target, keyed by address pattern */
    using MessageQueue = std::map<juce::String, juce::OSCMessage>;

    //==========================================================================
    // Private Members
    //==========================================================================

    int maxRateHz;
    int minIntervalMs;

    // Per-target message queues
    std::array<MessageQueue, MAX_TARGETS> targetQueues;

    // Broadcast queue (sent to all targets)
    MessageQueue broadcastQueue;

    // Last send time per target
    std::array<juce::int64, MAX_TARGETS> lastSendTime;

    // Thread safety
    juce::CriticalSection queueLock;

    // Callback
    SendCallback onSend;

    // Statistics
    std::atomic<int64_t> totalSent { 0 };
    std::atomic<int64_t> totalCoalesced { 0 };

    //==========================================================================
    // Private Methods
    //==========================================================================

    void processTarget(int targetIndex);
    void processBroadcast();
    bool canSendToTarget(int targetIndex) const;

    /**
     * Build the coalescing key for a message.
     * For messages with an integer first argument (channel ID), the key is "address:channelId".
     * This ensures messages for different channels are not coalesced together.
     * For messages without arguments or non-integer first argument, the key is just the address.
     */
    static juce::String buildCoalescingKey(const juce::OSCMessage& message);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSCRateLimiter)
};

} // namespace WFSNetwork
