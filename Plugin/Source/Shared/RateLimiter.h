#pragma once

#include <functional>
#include <mutex>
#include <unordered_map>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

namespace wfs::plugin
{
    class RateLimiter : private juce::Timer
    {
    public:
        using SendFn = std::function<void (const juce::String& /*path*/,
                                           int                /*channelId*/,
                                           float              /*value*/)>;

        RateLimiter();
        ~RateLimiter() override;

        void setSendFunction (SendFn fn);
        void setWindowHz (double hz);

        void post (const juce::String& path,
                   int channelId,
                   float value,
                   float epsilon);

    private:
        void timerCallback() override;

        struct Entry
        {
            float pending = 0.0f;
            float lastSent = 0.0f;
            float epsilon = 0.0f;
            bool  hasPending = false;
            bool  hasLast = false;
        };

        SendFn sendFn;
        double windowHz = 50.0;
        std::mutex lock;
        std::unordered_map<juce::String, std::unordered_map<int, Entry>> entries;
    };
}
