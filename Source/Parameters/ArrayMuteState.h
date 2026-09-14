#pragma once

#include <JuceHeader.h>
#include "WFSParameterDefaults.h"
#include <array>
#include <atomic>

/**
 * Per-array output mute — SESSION STATE, deliberately not a parameter.
 *
 * One flag per output array (1..outputArrayMax). An output is silenced while the
 * array it currently belongs to is muted, so moving a speaker into a muted array
 * mutes it at once and moving it to Single unmutes it. Single outputs have no
 * array and cannot be muted here.
 *
 * It lives outside the ValueTree on purpose: it is never saved, never undoable
 * and never carried by a snapshot, like the reverb Mute Pre/Post. A project or
 * output-config load clears it (a stale mute must not silence a freshly opened
 * show); an input snapshot recall does not (a cue must not unmute an array the
 * operator muted mid-show).
 *
 * Written on the message thread only (the Outputs tab, and the tablet's
 * /arrayAdjust/mute after OSCManager's callAsync hop); listeners are called
 * synchronously there. The flags are atomics so any thread may read them.
 */
class ArrayMuteState
{
public:
    static constexpr int numArrays = WFSParameterDefaults::outputArrayMax;

    struct Listener
    {
        virtual ~Listener() = default;
        virtual void arrayMuteChanged (int arrayId, bool muted) = 0;
    };

    /** @param arrayId 1..numArrays; anything else reads as unmuted. */
    bool isMuted (int arrayId) const noexcept
    {
        return isValidArray (arrayId) && flags[(size_t) (arrayId - 1)].load (std::memory_order_relaxed);
    }

    /** @return false when arrayId is out of range (nothing changes). */
    bool setMuted (int arrayId, bool shouldBeMuted)
    {
        JUCE_ASSERT_MESSAGE_THREAD
        if (! isValidArray (arrayId))
            return false;

        auto& flag = flags[(size_t) (arrayId - 1)];
        if (flag.exchange (shouldBeMuted, std::memory_order_relaxed) != shouldBeMuted)
            listeners.call ([arrayId, shouldBeMuted] (Listener& l) { l.arrayMuteChanged (arrayId, shouldBeMuted); });
        return true;
    }

    void clearAll()
    {
        for (int a = 1; a <= numArrays; ++a)
            setMuted (a, false);
    }

    bool anyMuted() const noexcept
    {
        for (auto& f : flags)
            if (f.load (std::memory_order_relaxed))
                return true;
        return false;
    }

    static bool isValidArray (int arrayId) noexcept { return arrayId >= 1 && arrayId <= numArrays; }

    void addListener (Listener* l)    { listeners.add (l); }
    void removeListener (Listener* l) { listeners.remove (l); }

private:
    std::array<std::atomic<bool>, (size_t) numArrays> flags {};
    juce::ListenerList<Listener> listeners;

    // A listener that can outlive this state (the Outputs tab: MainComponent's
    // tabbedComponent is destroyed after WfsParameters) holds a WeakReference and
    // unregisters only while it is still alive.
    JUCE_DECLARE_WEAK_REFERENCEABLE (ArrayMuteState)
};
