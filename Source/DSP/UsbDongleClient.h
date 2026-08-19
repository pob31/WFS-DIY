#pragma once

#include <JuceHeader.h>
#include "UsbHeadTrackerSource.h"
#include "../WFSLogger.h"
#include "../AppSettings.h"
#include "headtracker/client.hpp"
#include "headtracker/transport.hpp"
#include <atomic>
#include <functional>
#include <memory>

/**
 * UsbDongleClient
 *
 * Owns the htk::Client session with the USB receiver dongle and routes each
 * tracker's ORIENT packets to its UsbHeadTrackerSource.
 *
 * Unlike the webcam, the dongle runs whenever it is present rather than only
 * while a tracker is selected: the tracker list can only be built from a live
 * stream, so the port has to be open before the user has anything to pick.
 * The cost is small (208 Hz x 26 B per tracker) but the port is held
 * exclusively while the app runs.
 *
 * Threading:
 *  - htk::Client's callbacks fire on ITS reader thread. onOrient does the
 *    latency-critical work there and hands off through RtSnapshot; anything
 *    touching the source list marshals to the message thread first.
 *  - New trackers are only ever materialised on the message thread, from the
 *    2 s timer, because creating one appends to HeadTrackerManager::sources.
 *  - The routing table is append-only with an atomic count, so the reader
 *    thread can walk it without a lock and never sees a half-written entry.
 *
 * Lifetime: shutdown() must complete before the UsbHeadTrackerSource objects
 * are destroyed — the reader thread calls into them. HeadTrackerManager
 * guarantees this both by declaration order and by calling shutdown()
 * explicitly.
 */
class UsbDongleClient : private juce::Timer
{
public:
    /** Creates a source for a newly discovered tracker and returns a pointer
        to it. Called on the message thread only; the manager owns the object. */
    using SourceFactory = std::function<UsbHeadTrackerSource*(uint16_t)>;

    explicit UsbDongleClient (SourceFactory factory) : createSource (std::move (factory)) {}

    ~UsbDongleClient() override { shutdown(); }

    /** Message thread. Opens the dongle (or a replay capture) and starts
        discovery. Silent no-op when disabled by settings. */
    void start()
    {
        if (! AppSettings::getHeadtrackUsbEnabled())
        {
            WFSLogger::getInstance().logInfo (
                "Head tracker: USB support disabled ('headtrackUsbEnabled' in WFS-DIY.settings)");
            return;
        }

        client.on_orient = [this] (const htk_orient& o) { routeOrient (o); };

        client.on_state = [this] (htk::ConnState s, const std::string& detail)
        {
            // Reader thread → message thread. The alive guard covers the
            // window where this object is destroyed with a callAsync in
            // flight (the same idiom as spatcore ControllerDevice).
            auto guard = alive;
            const juce::String text (detail);
            const bool down = (s != htk::ConnState::Ready);
            juce::MessageManager::callAsync ([this, guard, s, text, down]
            {
                if (! *guard)
                    return;
                logStateChange (s, text);
                if (down)
                    for (int i = 0; i < slotCount.load (std::memory_order_acquire); ++i)
                        slots[i].load (std::memory_order_acquire)->onLinkDown();
            });
        };

        client.on_log = [] (std::string_view s)
        {
            WFSLogger::getInstance().logInfo ("Head tracker dongle: " + juce::String (std::string (s)));
        };

        openSession();
        startTimer (2000);   // discovery + reconnect, the ControllerDevice cadence
    }

    /** Message thread. Stops the reader thread and joins it. Must run before
        the sources it routes to are destroyed. Safe to call twice. */
    void shutdown()
    {
        stopTimer();
        *alive = false;
        client.stop();
    }

    /** True while a dongle session is up (message thread; also drives the
        "is this source real" question for the UI). */
    bool isConnected() const { return client.state() == htk::ConnState::Ready; }

private:
    void openSession()
    {
        const juce::String replay = AppSettings::getHeadtrackUsbReplayFile();
        const juce::String port   = AppSettings::getHeadtrackUsbPort();

        htk::Client::Options opts;

        std::unique_ptr<htk::Transport> transport;
        juce::String target;

        if (replay.isNotEmpty())
        {
            // Hardware-free path: replay an htgen capture as though it were a
            // dongle. This is what makes the whole app-side chain testable
            // before any silicon exists.
            transport = htk::make_file (208.0 * 31.0, true /*loop*/);
            target = replay;
            opts.no_data_watchdog_ms = 0;   // pacing means legitimate quiet gaps
            WFSLogger::getInstance().logInfo (
                "Head tracker: replaying '" + replay + "' ('headtrackUsbReplayFile' in WFS-DIY.settings)");
        }
        else
        {
            transport = htk::make_serial();
            target = port;   // empty = auto-discover
        }

        if (! client.start (std::move (transport), target.toStdString(), opts))
            WFSLogger::getInstance().logWarning ("Head tracker: dongle session already running");
    }

    void logStateChange (htk::ConnState s, const juce::String& detail)
    {
        const juce::String what = juce::String (htk::to_string (s))
                                + (detail.isEmpty() ? juce::String() : " - " + detail);
        if (s == htk::ConnState::Ready)
        {
            const auto d = client.device();
            WFSLogger::getInstance().logInfo (
                "Head tracker: dongle ready on " + juce::String (d.port)
                + " (firmware " + juce::String (d.fw_major) + "." + juce::String (d.fw_minor)
                + "." + juce::String (d.fw_patch) + ", protocol " + juce::String (d.proto_ver) + ")");
        }
        else if (s == htk::ConnState::Incompatible)
        {
            WFSLogger::getInstance().logWarning ("Head tracker: " + detail
                + " - update the dongle firmware or the application");
        }
        else if (s == htk::ConnState::Lost)
        {
            // Not a warning on the no-dongle-present path: most installations
            // have no tracker and should not see this every few seconds.
            WFSLogger::getInstance().logInfo ("Head tracker: " + what);
        }
    }

    /** Reader thread. Walks the append-only routing table; ids with no source
        yet are ignored until the timer materialises one. */
    void routeOrient (const htk_orient& o) noexcept
    {
        const int n = slotCount.load (std::memory_order_acquire);
        for (int i = 0; i < n; ++i)
        {
            auto* s = slots[i].load (std::memory_order_acquire);
            if (s != nullptr && s->getTrackerId() == o.id)
            {
                s->onOrient (o);
                return;
            }
        }
    }

    /** Message thread. Creates a source for any tracker the dongle now knows
        about, and reconnects the session if it dropped. */
    void timerCallback() override
    {
        if (! client.running())
        {
            openSession();
            return;
        }

        for (const auto& t : client.trackers())
        {
            if (findSlot (t.id) != nullptr)
                continue;

            const int n = slotCount.load (std::memory_order_relaxed);
            if (n >= kMaxTrackers)
            {
                WFSLogger::getInstance().logWarning (
                    "Head tracker: ignoring tracker beyond the " + juce::String (kMaxTrackers)
                    + " supported in one session");
                break;
            }

            if (auto* source = createSource (t.id))
            {
                // Publish the pointer before the count, so a reader that sees
                // the new count always sees a fully constructed entry.
                slots[n].store (source, std::memory_order_release);
                slotCount.store (n + 1, std::memory_order_release);

                WFSLogger::getInstance().logInfo (
                    "Head tracker: found " + source->getDisplayName()
                    + (t.vbat_mV > 0 ? " (battery " + juce::String (t.vbat_mV / 1000.0, 2) + " V)"
                                     : juce::String()));
            }
        }

        logStreamingTransitions();
    }

    /** "Is my tracker actually sending?" is the first question at bring-up and
        the first question when something stops working, and the attitude
        readout only answers it for the ONE selected tracker. isConnected() is
        driven by sample arrival, so a transition here means real ORIENT
        packets reached that source. */
    void logStreamingTransitions()
    {
        const int n = slotCount.load (std::memory_order_acquire);
        for (int i = 0; i < n; ++i)
        {
            auto* s = slots[i].load (std::memory_order_acquire);
            if (s == nullptr)
                continue;

            const bool streaming = s->isConnected();
            if (streaming != wasStreaming[i])
            {
                wasStreaming[i] = streaming;
                WFSLogger::getInstance().logInfo (
                    "Head tracker: " + s->getDisplayName()
                    + (streaming ? " streaming" : " stopped streaming"));
            }

            // Reference-frame automation state. This is the readout the wear
            // threshold is calibrated against: auto-level only learns while
            // the unit reads as WORN, so "never worn" and "worn while sitting
            // on the desk" are the two failure modes to watch for.
            const auto st = s->getStabilizerStatus();
            if (st.worn != wasWorn[i])
            {
                wasWorn[i] = st.worn;
                WFSLogger::getInstance().logInfo (
                    "Head tracker: " + s->getDisplayName()
                    + (st.worn ? " worn" : " not worn"));
            }
            if (st.level_ready != wasLevelReady[i])
            {
                wasLevelReady[i] = st.level_ready;
                if (st.level_ready)
                    WFSLogger::getInstance().logInfo (
                        "Head tracker: " + s->getDisplayName() + " auto-level engaged (mount tilt "
                        + juce::String (st.tilt_deg, 1) + juce::String::fromUTF8 ("Â°")
                        + ", confidence " + juce::String (st.level_confidence, 2) + ")");
            }

            // A tap is a deliberate user action on the head unit itself, so
            // confirming it registered matters more than most telemetry: there
            // is no on-device feedback beyond the LED blip.
            //
            // Reported as DETECTED rather than as recentred, because the two
            // are no longer the same event. on_tap fires on every detected tap
            // for observability, but the library honours the action only for a
            // wearer who has been settled for a couple of seconds (donning
            // headphones is a wear transition full of tap-like jostles), and
            // even then defers the recenter until the head stops moving, so
            // "front" is not captured mid-swing.
            const auto taps = s->getTapCount();
            if (taps != lastTaps[i])
            {
                lastTaps[i] = taps;
                WFSLogger::getInstance().logInfo (
                    "Head tracker: " + s->getDisplayName() + " double-tap detected"
                    + juce::String (st.worn ? "" : " - ignored, not worn"));
            }
        }
    }

    UsbHeadTrackerSource* findSlot (uint16_t id) const
    {
        const int n = slotCount.load (std::memory_order_acquire);
        for (int i = 0; i < n; ++i)
            if (auto* s = slots[i].load (std::memory_order_acquire); s != nullptr && s->getTrackerId() == id)
                return s;
        return nullptr;
    }

    // The protocol recommends reducing per-tracker rates beyond ~4 concurrent
    // trackers, so this is far above any realistic rig.
    static constexpr int kMaxTrackers = 16;

    SourceFactory createSource;
    htk::Client client;

    std::atomic<UsbHeadTrackerSource*> slots[kMaxTrackers] {};
    std::atomic<int> slotCount { 0 };
    bool wasStreaming[kMaxTrackers] {};    // all message thread only
    bool wasWorn[kMaxTrackers] {};
    bool wasLevelReady[kMaxTrackers] {};
    uint32_t lastTaps[kMaxTrackers] {};

    // Shared with in-flight callAsync lambdas; cleared in shutdown().
    std::shared_ptr<bool> alive { std::make_shared<bool> (true) };
};
