#pragma once

#include <JuceHeader.h>
#include "AppSettings.h"
#include <atomic>
#include <map>
#include <tuple>
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
 */
class MidiSnapshotTrigger final : private juce::MidiInputCallback
{
public:
    /** Note-on velocity must be STRICTLY greater than this to fire. */
    static constexpr int kVelocityThreshold  = 64;

    /** The same key is ignored inside this window (held pad, MIDI echo,
        doubled cable). A different bound note always fires immediately. */
    static constexpr int kRetriggerLockoutMs = 250;

    MidiSnapshotTrigger()
    {
        deviceListConnection = juce::MidiDeviceListConnection::make ([this] { onDeviceListChanged(); });

        desiredIdentifier = AppSettings::getMidiSnapshotInputId();
        desiredName       = AppSettings::getMidiSnapshotInputName();

        reopenIfNeeded();
    }

    ~MidiSnapshotTrigger() override { closePort(); }

    //==========================================================================
    // Message thread only
    //==========================================================================

    static juce::Array<juce::MidiDeviceInfo> getAvailableDevices()
    {
        return juce::MidiInput::getAvailableDevices();
    }

    juce::String getSelectedIdentifier() const { return desiredIdentifier; }
    juce::String getSelectedName()       const { return desiredName; }

    /** True when a port is actually open. False with a non-empty identifier
        means the device is absent, or present but refused to open (on Windows
        a MIDI port cannot be opened twice). */
    bool isPortOpen() const noexcept { return input != nullptr; }

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
        order given (the scan sorts by file name), and every loser is reported
        through onDuplicateBinding.

        The UI refuses to CREATE a collision; this is the safety net for one
        that arrives on disk -- a copied snapshot file, hand-edited XML, or a
        restored backup -- so it still behaves predictably instead of firing
        arbitrarily. */
    void setBindings (const std::vector<std::tuple<int, int, juce::String>>& bindings)
    {
        keyToSnapshot.clear();

        for (int ch = 1; ch <= 16; ++ch)
            for (int n = 0; n < 128; ++n)
                boundNotes[ch][n].store (0, std::memory_order_relaxed);

        for (const auto& [ch, note, snapName] : bindings)
        {
            if (ch < 1 || ch > 16 || note < 0 || note > 127 || snapName.isEmpty())
                continue;

            const int key = packKey (ch, note);

            if (auto it = keyToSnapshot.find (key); it != keyToSnapshot.end())
            {
                if (onDuplicateBinding)
                    onDuplicateBinding (ch, note, it->second, snapName);
                continue;  // first in file order wins
            }

            keyToSnapshot[key] = snapName;
            boundNotes[ch][note].store (1, std::memory_order_release);
        }

        // A stale parked key could name a note that has just been rebound.
        pendingKey.store (-1, std::memory_order_relaxed);
    }

    /** Returns the packed key of a pending recall, or -1 for nothing pending. */
    int takePendingRecall() noexcept
    {
        return pendingKey.exchange (-1, std::memory_order_acquire);
    }

    /** Snapshot name bound to a packed key, or empty. */
    juce::String resolve (int key) const
    {
        auto it = keyToSnapshot.find (key);
        return it == keyToSnapshot.end() ? juce::String() : it->second;
    }

    static int packKey  (int channel, int note) noexcept { return (channel << 8) | note; }
    static int channelOf (int key) noexcept { return key >> 8; }
    static int noteOf    (int key) noexcept { return key & 0xff; }

    /** (channel, note, winner, loser) -- message thread. */
    std::function<void (int, int, const juce::String&, const juce::String&)> onDuplicateBinding;

    /** MIDI device list changed (hot-plug) -- message thread. */
    std::function<void()> onDeviceListRefreshed;

private:
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
        // replug permanently fatal. Close it first when it is gone.
        if (input != nullptr)
        {
            const auto devices = juce::MidiInput::getAvailableDevices();
            const auto openId  = input->getIdentifier();

            bool stillPresent = false;
            for (const auto& d : devices)
                if (d.identifier == openId) { stillPresent = true; break; }

            if (! stillPresent)
                closePort();
        }

        reopenIfNeeded();

        if (onDeviceListRefreshed)
            onDeviceListRefreshed();
    }

    void closePort()
    {
        if (input != nullptr)
        {
            input->stop();
            input.reset();
        }
    }

    /** Identifier first, then name -- MidiDeviceInfo::identifier is OS-formatted
        and not promised stable across reboots. */
    void reopenIfNeeded()
    {
        if (input != nullptr || desiredIdentifier.isEmpty())
            return;

        const auto devices = juce::MidiInput::getAvailableDevices();

        juce::String openId;

        for (const auto& d : devices)
            if (d.identifier == desiredIdentifier) { openId = d.identifier; break; }

        if (openId.isEmpty() && desiredName.isNotEmpty())
        {
            for (const auto& d : devices)
            {
                if (d.name == desiredName)
                {
                    openId = d.identifier;
                    desiredIdentifier = d.identifier;              // adopt the new identifier
                    AppSettings::setMidiSnapshotInputId (openId);
                    break;
                }
            }
        }

        if (openId.isEmpty())
            return;  // not plugged in -- stay selected, retry on hot-plug

        input = juce::MidiInput::openDevice (openId, this);

        if (input != nullptr)
            input->start();
        else
            DBG ("MIDI snapshot trigger: could not open " + desiredName);
    }

    std::atomic<uint8_t>      boundNotes[17][128] {};  // [1..16][0..127]; 2 KB
    std::atomic<int>          pendingKey   { -1 };
    std::atomic<int>          lastFiredKey { -1 };
    std::atomic<juce::uint32> lastFiredMs  { 0 };

    std::map<int, juce::String> keyToSnapshot;  // MESSAGE THREAD ONLY

    std::unique_ptr<juce::MidiInput> input;
    juce::MidiDeviceListConnection   deviceListConnection;
    juce::String desiredIdentifier, desiredName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiSnapshotTrigger)
};
