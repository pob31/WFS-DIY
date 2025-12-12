#include "OSCRateLimiter.h"

namespace WFSNetwork
{

//==============================================================================
// Construction/Destruction
//==============================================================================

OSCRateLimiter::OSCRateLimiter(int rateHz)
    : maxRateHz(rateHz)
    , minIntervalMs(1000 / rateHz)
{
    // Initialize last send times to 0
    lastSendTime.fill(0);

    // Start the timer at roughly 2x the send rate for responsiveness
    startTimer(minIntervalMs / 2);
}

OSCRateLimiter::~OSCRateLimiter()
{
    stopTimer();
}

//==============================================================================
// Configuration
//==============================================================================

void OSCRateLimiter::setMaxRate(int hz)
{
    const juce::ScopedLock sl(queueLock);

    maxRateHz = juce::jmax(1, hz);
    minIntervalMs = 1000 / maxRateHz;

    // Restart timer with new interval
    stopTimer();
    startTimer(minIntervalMs / 2);
}

//==============================================================================
// Message Queueing
//==============================================================================

void OSCRateLimiter::queueMessage(int targetIndex, const juce::OSCMessage& message)
{
    if (targetIndex < -1 || targetIndex >= MAX_TARGETS)
        return;

    const juce::ScopedLock sl(queueLock);

    juce::String address = message.getAddressPattern().toString();

    if (targetIndex == -1)
    {
        // Broadcast to all targets
        queueBroadcast(message);
    }
    else
    {
        auto& queue = targetQueues[static_cast<size_t>(targetIndex)];

        // Check if we're replacing an existing message (coalescing)
        if (queue.find(address) != queue.end())
        {
            ++totalCoalesced;
        }

        // Store/replace the message (use insert_or_assign to avoid default construction)
        queue.insert_or_assign(address, message);
    }
}

void OSCRateLimiter::queueBroadcast(const juce::OSCMessage& message)
{
    const juce::ScopedLock sl(queueLock);

    juce::String address = message.getAddressPattern().toString();

    // Check if we're replacing an existing message
    if (broadcastQueue.find(address) != broadcastQueue.end())
    {
        ++totalCoalesced;
    }

    // Use insert_or_assign to avoid default construction
    broadcastQueue.insert_or_assign(address, message);
}

void OSCRateLimiter::flushAll()
{
    const juce::ScopedLock sl(queueLock);

    if (!onSend)
        return;

    // Send all queued messages immediately

    // Process broadcast queue first
    for (auto& [address, message] : broadcastQueue)
    {
        for (int i = 0; i < MAX_TARGETS; ++i)
        {
            onSend(i, message);
            ++totalSent;
        }
    }
    broadcastQueue.clear();

    // Process per-target queues
    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        auto& queue = targetQueues[static_cast<size_t>(i)];
        for (auto& [address, message] : queue)
        {
            onSend(i, message);
            ++totalSent;
        }
        queue.clear();
        lastSendTime[static_cast<size_t>(i)] = juce::Time::currentTimeMillis();
    }
}

void OSCRateLimiter::clearAll()
{
    const juce::ScopedLock sl(queueLock);

    broadcastQueue.clear();
    for (auto& queue : targetQueues)
        queue.clear();
}

//==============================================================================
// Statistics
//==============================================================================

int OSCRateLimiter::getPendingCount() const
{
    const juce::ScopedLock sl(queueLock);

    int count = static_cast<int>(broadcastQueue.size());
    for (const auto& queue : targetQueues)
        count += static_cast<int>(queue.size());

    return count;
}

void OSCRateLimiter::resetStats()
{
    totalSent = 0;
    totalCoalesced = 0;
}

//==============================================================================
// Timer Callback
//==============================================================================

void OSCRateLimiter::timerCallback()
{
    const juce::ScopedLock sl(queueLock);

    if (!onSend)
        return;

    // Process broadcast queue (sends to all targets that can receive)
    processBroadcast();

    // Process each target's queue
    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        if (canSendToTarget(i))
        {
            processTarget(i);
        }
    }
}

//==============================================================================
// Private Methods
//==============================================================================

void OSCRateLimiter::processTarget(int targetIndex)
{
    auto& queue = targetQueues[static_cast<size_t>(targetIndex)];

    if (queue.empty())
        return;

    // Send all pending messages for this target
    for (auto& [address, message] : queue)
    {
        onSend(targetIndex, message);
        ++totalSent;
    }

    queue.clear();
    lastSendTime[static_cast<size_t>(targetIndex)] = juce::Time::currentTimeMillis();
}

void OSCRateLimiter::processBroadcast()
{
    if (broadcastQueue.empty())
        return;

    // For broadcast, send to all targets that can receive
    for (auto& [address, message] : broadcastQueue)
    {
        for (int i = 0; i < MAX_TARGETS; ++i)
        {
            if (canSendToTarget(i))
            {
                onSend(i, message);
                ++totalSent;
            }
        }
    }

    broadcastQueue.clear();

    // Update all send times
    auto now = juce::Time::currentTimeMillis();
    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        lastSendTime[static_cast<size_t>(i)] = now;
    }
}

bool OSCRateLimiter::canSendToTarget(int targetIndex) const
{
    if (targetIndex < 0 || targetIndex >= MAX_TARGETS)
        return false;

    auto now = juce::Time::currentTimeMillis();
    auto lastTime = lastSendTime[static_cast<size_t>(targetIndex)];

    return (now - lastTime) >= minIntervalMs;
}

} // namespace WFSNetwork
