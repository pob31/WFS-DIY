#pragma once

#include <juce_core/juce_core.h>

namespace wfs::plugin
{
    // Small thread-safe ring of recent diagnostic lines. Processor writers
    // call add() from any thread; editors poll snapshot() on a timer and
    // redraw when the version counter changes.
    class DiagnosticLog
    {
    public:
        static constexpr int kMaxLines = 4;

        void add (const juce::String& line)
        {
            const juce::ScopedLock sl (lock);
            const auto stamped = juce::Time::getCurrentTime()
                                     .toString (false, true, true, true)
                                 + "  " + line;
            lines.add (stamped);
            while (lines.size() > kMaxLines)
                lines.remove (0);
            version.fetch_add (1, std::memory_order_release);
        }

        juce::StringArray snapshot() const
        {
            const juce::ScopedLock sl (lock);
            return lines;
        }

        int getVersion() const noexcept
        {
            return version.load (std::memory_order_acquire);
        }

    private:
        juce::CriticalSection lock;
        juce::StringArray lines;
        std::atomic<int> version { 0 };
    };
}
