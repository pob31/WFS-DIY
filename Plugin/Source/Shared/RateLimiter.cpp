#include "RateLimiter.h"

namespace wfs::plugin
{
    RateLimiter::RateLimiter() = default;

    RateLimiter::~RateLimiter()
    {
        stopTimer();
    }

    void RateLimiter::setSendFunction (SendFn fn)
    {
        std::lock_guard<std::mutex> sl (lock);
        sendFn = std::move (fn);
    }

    void RateLimiter::setWindowHz (double hz)
    {
        windowHz = juce::jlimit (1.0, 500.0, hz);
        if (isTimerRunning())
            startTimerHz (static_cast<int> (windowHz));
    }

    void RateLimiter::post (const juce::String& path,
                            int channelId,
                            float value,
                            float epsilon)
    {
        {
            std::lock_guard<std::mutex> sl (lock);
            auto& e = entries[path][channelId];
            e.pending    = value;
            e.epsilon    = epsilon;
            e.hasPending = true;
        }

        // Start the timer lazily on first post — avoids a running timer
        // during host scan, which some strict live-sound hosts (e.g.
        // Live Professor) treat as a scan failure.
        if (! isTimerRunning())
            startTimerHz (static_cast<int> (windowHz));
    }

    void RateLimiter::timerCallback()
    {
        std::vector<std::tuple<juce::String, int, float>> toSend;
        {
            std::lock_guard<std::mutex> sl (lock);
            for (auto& [path, perChan] : entries)
            {
                for (auto& [chan, e] : perChan)
                {
                    if (! e.hasPending)
                        continue;

                    const bool isFirst     = ! e.hasLast;
                    const float delta      = std::abs (e.pending - e.lastSent);
                    const bool changed     = isFirst || delta > e.epsilon;

                    if (changed)
                    {
                        toSend.emplace_back (path, chan, e.pending);
                        e.lastSent   = e.pending;
                        e.hasLast    = true;
                    }
                    e.hasPending = false;
                }
            }
        }

        if (sendFn)
            for (auto& [path, chan, val] : toSend)
                sendFn (path, chan, val);
    }
}
