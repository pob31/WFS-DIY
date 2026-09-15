#pragma once

#include <JuceHeader.h>
#include "AppSettings.h"
#include <atomic>
#include <map>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

/**
 * MidiSnapshotTrigger
 *
 * Owns ONE juce::MidiInput and turns note-ons above the velocity threshold into
 * a pending snapshot recall, parked in an atomic for the message thread to drain.
 *
 * WHY IT OWNS THE PORT ITSELF (rather than AudioDeviceManager::
 * setMidiInputDeviceEnabled + addMidiInputDeviceCallback):
 *   - That route persists the selection into the "audioDeviceState" blob, which
 *     is only replayed on the XML fast path (DeviceHost::restoreFromXml ->
 *     AudioDeviceManager::initialise). The openNamedDevice FALLBACK in
 *     MainComponent never calls initialise(), and a first launch with no saved
 *     state never calls it at all -- i.e. MIDI would silently die on exactly the
 *     launches where the audio device failed and the operator most needs the
 *     show to keep running.
 *   - addMidiInputDeviceCallback silently no-ops for a device that is not
 *     already enabled; owning the port removes that ordering trap entirely.
 *   - setMidiInputDeviceEnabled fires sendChangeMessage, which re-enters
 *     MainComponent::changeListenerCallback and its channel-count / callback
 *     work. Not something a MIDI selection should trigger.
 *
 * WHY IT LIVES IN MainComponent, NOT IN AudioInterfaceWindow:
 *   openAudioInterfaceWindow() early-returns while processing is enabled and
 *   handleProcessingChange force-hides the window when processing starts; the
 *   window is also lazily constructed on first open. A listener living there
 *   would be absent during a show -- the only time this feature matters. The
 *   window owns the SELECTOR; this owns the port and the binding index.
 *
 * THREADING:
 *   - handleIncomingMidiMessage runs on a high-priority OS MIDI thread. It does
 *     not allocate, lock, log, touch a ValueTree or touch a Component. It ends
 *     in one release-store.
 *   - Deliberately NO MessageManager::callAsync: a recall costs tens of ms
 *     (handleConfigReloaded re-rasterises every gradient map, recomputes the
 *     whole matrix and refreshes five tabs), so a burst would queue dozens of
 *     full reloads the operator has already moved past. The owner POLLS
 *     takePendingRecall() from its existing 5 ms timer instead, which gives
 *     latest-wins coalescing for free.
 *   - Because the drain is a poll, nothing here captures `this` and no
 *     shared_ptr<bool> alive guard is needed.
 *   - Device enumeration and MidiDeviceListConnection are message-thread only;
 *     JUCE delivers the list-change callback on the message thread.
 *
 * CO-EXISTENCE: roli_blocks_basics independently enumerates and opens MIDI
 * ports but filters on isBlocksMidiDeviceName, so it will not contend for a
 * generic controller. A Lightpad's own port WILL appear in the selector and
 * must not be chosen -- on Windows a port cannot be opened twice.
 *
 * PORT HEALTH: a device that drops off the bus and comes back inside JUCE's
 * 500 ms device-change debounce leaves the device list identical, so
 * MidiDeviceListConnection never fires and the open MidiInput keeps the dead
 * instance's handle for good. Every raw OS device-change notification
 * therefore reopens the port (RawMidiDeviceChangeFlag). A port that is listed
 * but refuses to open (held by another application) is retried every 2 s
 * rather than looking armed while dead.
 */
class MidiSnapshotTrigger final : private juce::MidiInputCallback,
                                  private juce::Timer
{
public:
    /** Note-on velocity must be STRICTLY greater than this to fire. */
    static constexpr int kVelocityThreshold  = 64;

    /** The same key is ignored inside this window (held pad, MIDI echo,
        doubled cable). A different bound note always fires immediately. */
    static constexpr int kRetriggerLockoutMs = 250;

    /** Retry interval for a listed port that refused to open. */
    static constexpr int kOpenRetryMs = 2000;

    enum class PortState
    {
        off,       // no device selected
        absent,    // selected, not plugged in
        open,      // receiving
        refused    // listed, but the OS refused to open it (in use elsewhere)
    };

    MidiSnapshotTrigger()
    {
        deviceListConnection = juce::MidiDeviceListConnection::make ([this] { onDeviceListChanged(); });
        RawMidiDeviceChangeFlag::get();   // registers with the OS change feed once

        desiredIdentifier = AppSettings::getMidiSnapshotInputId();
        desiredName       = AppSettings::getMidiSnapshotInputName();

        reopenIfNeeded();
        startTimer (250);
    }

    ~MidiSnapshotTrigger() override
    {
        stopTimer();
        closePort();
    }

    //==========================================================================
    // Message thread only
    //==========================================================================

    static juce::Array<juce::MidiDeviceInfo> getAvailableDevices()
    {
        return juce::MidiInput::getAvailableDevices();
    }

    /** The saved selection. The port actually open can differ: see getOpenIdentifier(). */
    juce::String getSelectedIdentifier() const { return desiredIdentifier; }
    juce::String getSelectedName()       const { return desiredName; }

    /** Identifier of the port actually open, or empty. It differs from the saved
        one when the name fallback found the device under a new identifier. */
    juce::String getOpenIdentifier() const { return input != nullptr ? openIdentifier : juce::String(); }

    /** True when a port is actually open. */
    bool isPortOpen() const noexcept { return input != nullptr; }

    PortState getPortState() const noexcept { return portState; }

    /** Empty identifier = off. Persists immediately, then (re)opens. */
    void selectDevice (const juce::String& identifier, const juce::String& deviceName)
    {
        closePort();

        desiredIdentifier = identifier;
        desiredName       = deviceName;

        AppSettings::setMidiSnapshotInputId (identifier);
        AppSettings::setMidiSnapshotInputName (deviceName);

        reopenIfNeeded();
    }

    /** Publish the binding table. Duplicates are resolved first-wins in the
        order given (the scan sorts by snapshot name), and each loser is
        reported through onDuplicateBinding once, when it first appears: the
        index is rebuilt on every recall, and repeating the warning there only
        buried it under the recall's own status line.

        The UI refuses to CREATE a collision; this is the safety net for one
        that arrives on disk -- a copied snapshot file, hand-edited XML, or a
        restored backup -- so it still behaves predictably instead of firing
        arbitrarily. */
    void setBindings (const std::vector<std::tuple<int, int, juce::String>>& bindings)
    {
        std::map<int, juce::String> next;
        std::set<std::pair<int, juce::String>> duplicates;
        bool bound[17][128] {};

        for (const auto& [ch, note, snapName] : bindings)
        {
            if (ch < 1 || ch > 16 || note < 0 || note > 127 || snapName.isEmpty())
                continue;

            const int key = packKey (ch, note);

            if (auto it = next.find (key); it != next.end())
            {
                duplicates.insert ({ key, snapName });
                if (reportedDuplicates.count ({ key, snapName }) == 0 && onDuplicateBinding)
                    onDuplicateBinding (ch, note, it->second, snapName);
                continue;  // first in snapshot-name order wins
            }

            next[key] = snapName;
            bound[ch][note] = true;
        }

        reportedDuplicates = std::move (duplicates);

        // One store per slot: clearing the whole table first left a window in
        // which a note bound both before and after the rebuild read as unbound.
        for (int ch = 1; ch <= 16; ++ch)
            for (int n = 0; n < 128; ++n)
                boundNotes[ch][n].store (bound[ch][n] ? 1 : 0, std::memory_order_release);

        // A note parked while the index was rebuilt keeps its place unless the
        // rebuild gave its key to another snapshot or unbound it -- it was
        // pressed for the snapshot the key named then. Every recall rebuilds
        // the index, so clearing it unconditionally threw away any cue pressed
        // while the previous recall was loading.
        const int parked = pendingKey.load (std::memory_order_acquire);
        if (parked >= 0 && ownerIn (keyToSnapshot, parked) != ownerIn (next, parked))
        {
            int expected = parked;   // a newer press is left alone
            pendingKey.compare_exchange_strong (expected, -1, std::memory_order_acq_rel);
        }

        keyToSnapshot = std::move (next);
    }

    /** Returns the packed key of a pending recall, or -1 for nothing pending. */
    int takePendingRecall() noexcept
    {
        return pendingKey.exchange (-1, std::memory_order_acquire);
    }

    /** Snapshot name bound to a packed key, or empty. */
    juce::String resolve (int key) const { return ownerIn (keyToSnapshot, key); }

    static int packKey  (int channel, int note) noexcept { return (channel << 8) | note; }
    static int channelOf (int key) noexcept { return key >> 8; }
    static int noteOf    (int key) noexcept { return key & 0xff; }

    /** (channel, note, winner, loser) -- message thread. */
    std::function<void (int, int, const juce::String&, const juce::String&)> onDuplicateBinding;

    /** (previous, current) -- message thread. For the owner's log and status bar. */
    std::function<void (PortState, PortState)> onPortStateChanged;

    /** For UI that shows the port (the Audio Interface selector) -- message thread. */
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void midiPortStateChanged() = 0;
    };

    void addListener (Listener* l)    { listeners.add (l); }
    void removeListener (Listener* l) { listeners.remove (l); }

private:
    /** Raised by every raw OS MIDI device-change notification, whether or not
        the device list changed. JUCE 9.0.2's ump::Endpoints::removeListener()
        calls addListener() (juce_UMPEndpoints.cpp:186), so a listener can never
        be detached: this one is registered once for the life of the process
        and only raises a flag, which the trigger polls. */
    struct RawMidiDeviceChangeFlag final : private juce::ump::EndpointsListener
    {
        static RawMidiDeviceChangeFlag& get()
        {
            static RawMidiDeviceChangeFlag instance;
            return instance;
        }

        bool take() noexcept { return raised.exchange (false, std::memory_order_acq_rel); }

    private:
        RawMidiDeviceChangeFlag()
        {
            if (auto* endpoints = juce::ump::Endpoints::getInstance())
                endpoints->addListener (*this);
        }

        void endpointsChanged() override { raised.store (true, std::memory_order_release); }

        std::atomic<bool> raised { false };
    };

    static juce::String ownerIn (const std::map<int, juce::String>& table, int key)
    {
        auto it = table.find (key);
        return it == table.end() ? juce::String() : it->second;
    }

    //==========================================================================
    // MIDI thread
    //==========================================================================

    void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& m) override
    {
        // MIDI THREAD. Nothing here may block, allocate or touch the app.
        if (! m.isNoteOn())                        return;  // velocity-0 note-on == note-off by default
        if (m.getVelocity() <= kVelocityThreshold) return;

        const int ch   = m.getChannel();  // 1..16, 0 when channel-less
        const int note = m.getNoteNumber();

        if (ch < 1 || ch > 16 || note < 0 || note > 127)
            return;

        if (boundNotes[ch][note].load (std::memory_order_acquire) == 0)
            return;

        const int  key = packKey (ch, note);
        const auto now = juce::Time::getMillisecondCounter();

        if (key == lastFiredKey.load (std::memory_order_relaxed)
            && now - lastFiredMs.load (std::memory_order_relaxed) < (juce::uint32) kRetriggerLockoutMs)
            return;

        lastFiredKey.store (key, std::memory_order_relaxed);
        lastFiredMs.store  (now, std::memory_order_relaxed);
        pendingKey.store   (key, std::memory_order_release);  // latest wins
    }

    //==========================================================================
    // Message thread
    //==========================================================================

    void onDeviceListChanged()
    {
        // JUCE does not null a MidiInput whose device has disappeared, so a
        // stale object would block every subsequent reopen and make unplug ->
        // replug permanently fatal. Close it first when it is gone -- and also
        // when it is the name fallback and the saved device has come back.
        if (input != nullptr
            && (! isListed (openIdentifier)
                || (openIdentifier != desiredIdentifier && isListed (desiredIdentifier))))
            closePort();

        reopenIfNeeded();
        listeners.call ([] (Listener& l) { l.midiPortStateChanged(); });
    }

    void timerCallback() override
    {
        // A raw device change can leave the open port dead even though the
        // device list reads the same before and after (a fast replug), so the
        // port is reopened on every one. Closing and reopening a healthy port
        // costs a few milliseconds.
        if (RawMidiDeviceChangeFlag::get().take())
        {
            closePort();
            reopenIfNeeded();
            listeners.call ([] (Listener& l) { l.midiPortStateChanged(); });
            return;
        }

        if (portState == PortState::refused
            && juce::Time::getMillisecondCounter() - lastOpenAttemptMs >= (juce::uint32) kOpenRetryMs)
            reopenIfNeeded();
    }

    void closePort()
    {
        if (input != nullptr)
        {
            input->stop();
            input.reset();
        }

        openIdentifier.clear();
    }

    bool isListed (const juce::String& identifier) const
    {
        if (identifier.isEmpty())
            return false;

        for (const auto& d : juce::MidiInput::getAvailableDevices())
            if (d.identifier == identifier)
                return true;

        return false;
    }

    /** Identifier first, then name -- MidiDeviceInfo::identifier is OS-formatted
        and not promised stable across reboots. The saved identifier is never
        replaced by the one the name fallback finds: with two controllers of
        the same name, that would move the trigger to the other unit for good. */
    void reopenIfNeeded()
    {
        if (input != nullptr)
            return;

        if (desiredIdentifier.isEmpty())
        {
            setPortState (PortState::off);
            return;
        }

        juce::String target;
        const auto devices = juce::MidiInput::getAvailableDevices();

        for (const auto& d : devices)
            if (d.identifier == desiredIdentifier) { target = d.identifier; break; }

        if (target.isEmpty() && desiredName.isNotEmpty())
            for (const auto& d : devices)
                if (d.name == desiredName) { target = d.identifier; break; }

        if (target.isEmpty())
        {
            setPortState (PortState::absent);  // stay selected, reopen on hot-plug
            return;
        }

        lastOpenAttemptMs = juce::Time::getMillisecondCounter();
        input = juce::MidiInput::openDevice (target, this);

        if (input != nullptr)
        {
            input->start();
            openIdentifier = target;
            setPortState (PortState::open);
        }
        else
        {
            setPortState (PortState::refused);  // retried by timerCallback
        }
    }

    void setPortState (PortState newState)
    {
        if (newState == portState)
            return;

        const auto previous = std::exchange (portState, newState);

        if (onPortStateChanged)
            onPortStateChanged (previous, newState);

        listeners.call ([] (Listener& l) { l.midiPortStateChanged(); });
    }

    std::atomic<uint8_t>      boundNotes[17][128] {};  // [1..16][0..127]; 2 KB
    std::atomic<int>          pendingKey   { -1 };
    std::atomic<int>          lastFiredKey { -1 };
    std::atomic<juce::uint32> lastFiredMs  { 0 };

    // MESSAGE THREAD ONLY from here down
    std::map<int, juce::String>            keyToSnapshot;
    std::set<std::pair<int, juce::String>> reportedDuplicates;

    std::unique_ptr<juce::MidiInput> input;
    juce::MidiDeviceListConnection   deviceListConnection;
    juce::String desiredIdentifier, desiredName, openIdentifier;
    PortState    portState = PortState::off;
    juce::uint32 lastOpenAttemptMs = 0;
    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiSnapshotTrigger)
};
