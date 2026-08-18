#pragma once

#include <JuceHeader.h>
#include "../../spatcore/binaural/HeadOrientationSource.h"
#include "../../spatcore/binaural/HeadFrame.h"
#include "../../spatcore/binaural/HeadAttitudePipeline.h"
#include "headtracker/orientation.hpp"
#include "headtracker/parser.hpp"
#include "headtracker/stabilizer.hpp"
#include <atomic>

/**
 * UsbHeadTrackerSource
 *
 * One head unit heard by the USB dongle, as a selectable head-orientation
 * source. The dongle multiplexes every tracker in range onto one serial
 * stream (headtracker PROTOCOL.md); UsbDongleClient demultiplexes by id and
 * hands each tracker's ORIENT packets to its own instance of this class.
 *
 * Reference frames are handled by htk::Stabilizer rather than by a bare
 * boresight, because the capture-instant tare this used to do was only ever
 * as good as the wearer's posture at the moment they clicked:
 *
 *  - AUTO-LEVEL derives the pitch/roll mounting tilt from the long-term
 *    average of body-frame gravity while the unit is worn. Gravity is
 *    invariant under any yaw manipulation - drift, recenter, the firmware's
 *    rest yaw-hold - so it is the one signal yaw bookkeeping cannot poison.
 *  - Mount AZIMUTH is NOT observable from gravity (a puck twisted about
 *    vertical looks identical), so yaw still needs a human action. That is
 *    what Set Zero and the head unit's double-tap are for.
 *  - AUTO-CENTER is left OFF deliberately: a WFS scene must not rotate
 *    unless the listener asked it to.
 *
 * Set Zero therefore means RECENTER (yaw only) rather than a full-pose tare:
 * with tilt self-calibrating there is nothing left for a full tare to fix,
 * and a yaw-only zero cannot bake in the pitch of someone who happened to be
 * looking down when they pressed it.
 *
 * Threading: the dongle's reader thread is the ONE producer (RtSnapshot
 * contract), and htk::Stabilizer::update() is reader-thread-only by the same
 * contract. request_*() and status() are safe from any thread.
 */
class UsbHeadTrackerSource : public spatcore::binaural::SnapshotHeadOrientationSource
{
public:
    UsbHeadTrackerSource (uint16_t trackerId,
                          const spatcore::binaural::HeadAttitudeTuning& tuning,
                          const htk::StabilizerConfig& stabilizerConfig)
        : id (trackerId), pipeline (tuning), stabilizer (stabilizerConfig)
    {
        idString = "usb:" + juce::String::toHexString ((int) trackerId).paddedLeft ('0', 4).toLowerCase();
        // The tracker id is derived from the head unit's factory device ID, so
        // it survives power cycles and is safe to persist in the show file.
        displayName = "Head tracker " + juce::String::toHexString ((int) trackerId)
                                            .paddedLeft ('0', 4).toUpperCase();

        // Fires on the reader thread when the wearer double-taps the head
        // unit. The Stabilizer has already recentred by the time this runs
        // (tap_recenters); this only counts it, so the message thread can
        // tell the user their tap registered.
        stabilizer.on_tap = [this] { tapCount.fetch_add (1, std::memory_order_release); };
    }

    juce::String getSourceId() const override    { return idString; }
    juce::String getDisplayName() const override { return displayName; }

    bool isConnected() const override
    {
        return juce::Time::getMillisecondCounter()
                 - lastSampleMs.load (std::memory_order_acquire) < kStaleMs;
    }

    spatcore::binaural::HeadOrientation getOrientation() const noexcept override
    {
        auto o = SnapshotHeadOrientationSource::getOrientation();
        // Mandatory, not belt-and-braces: when the whole dongle dies there is
        // no thread left to publish an invalid sample, so staleness has to be
        // decided on wall-clock age at the point of use.
        if (juce::Time::getMillisecondCounter()
              - lastSampleMs.load (std::memory_order_acquire) > kStaleMs)
            o.valid = false;
        return o;
    }

    void setZero() override
    {
        // Timestamped rather than handed straight to the Stabilizer: its
        // request bitmask is applied on the next update(), so pressing Set
        // Zero while the dongle is unplugged would otherwise fire minutes
        // later against whatever pose the wearer happened to hold, silently
        // rotating the scene. The request is forwarded in onOrient() only
        // while it is still fresh.
        auto now = juce::Time::getMillisecondCounter();
        zeroRequestedMs.store (now == 0 ? 1 : now, std::memory_order_release);
    }

    // The dongle is a SHARED device that must already be streaming before any
    // selection can be made (its stream is what the tracker list is built
    // from), so selection does not own its lifetime.
    bool activate() override { return true; }
    void deactivate() override {}

    uint16_t getTrackerId() const noexcept { return id; }

    /** Message thread. Reference-frame state for logging and the UI. */
    htk::StabilizerStatus getStabilizerStatus() const { return stabilizer.status(); }
    uint32_t getTapCount() const noexcept { return tapCount.load (std::memory_order_acquire); }

    //==========================================================================
    // Reader thread only.

    void onOrient (const htk_orient& o) noexcept
    {
        lastSampleMs.store (juce::Time::getMillisecondCounter(), std::memory_order_release);

        if (const auto requested = zeroRequestedMs.exchange (0, std::memory_order_acq_rel))
        {
            // Drop a stale request rather than applying it to an unrelated
            // pose after a reconnect. Consumed by the update() below.
            if (juce::Time::getMillisecondCounter() - requested < kZeroRequestWindowMs)
                stabilizer.request_recenter();
        }

        // The Stabilizer refuses non-finite wire values itself (counted in
        // status().dropped_nonfinite), so the guard here is on its OUTPUT:
        // whatever it hands back must be finite before it can latch the
        // smoothing filters, which carry their history forward forever.
        const htk::Quat corrected = stabilizer.update (o);
        if (! (std::isfinite (corrected.w) && std::isfinite (corrected.x)
               && std::isfinite (corrected.y) && std::isfinite (corrected.z)))
        {
            publishOrientation ({});
            return;
        }

        float yaw, pitch, roll;
        spatcore::binaural::headframe::trackerQuatToYawPitchRoll (
            corrected.w, corrected.x, corrected.y, corrected.z, yaw, pitch, roll);

        publishOrientation (pipeline.process (yaw, pitch, roll,
                                              juce::Time::getMillisecondCounterHiRes() * 0.001));
    }

    /** The dongle went away. Publish "nothing trustworthy" so the renderer
        slews back to the manual parameters, and drop a pending zero request
        so it cannot fire against a post-reconnect pose.

        The Stabilizer's learned level is deliberately KEPT: it took minutes of
        wear to earn, its wear gate already distrusts it after a large gravity
        step (pick-up/put-down), and discarding it on every dropout would mean
        never converging on a flaky link.  */
    void onLinkDown() noexcept
    {
        zeroRequestedMs.store (0, std::memory_order_release);
        lastSampleMs.store (0, std::memory_order_release);
        pipeline.reset();
        publishOrientation ({});
    }

private:
    // 208 Hz is one packet every 4.8 ms, so 150 ms is ~31 consecutive losses.
    // With ~3% air occupancy per tracker and no retransmissions that only
    // happens on real link loss, never on ordinary collision loss.
    // TODO(hardware): re-check against real air-loss statistics at bring-up.
    static constexpr uint32_t kStaleMs = 150;
    static constexpr uint32_t kZeroRequestWindowMs = 2000;

    const uint16_t id;
    juce::String idString, displayName;

    std::atomic<juce::uint32> zeroRequestedMs { 0 };
    std::atomic<juce::uint32> lastSampleMs { 0 };
    std::atomic<uint32_t> tapCount { 0 };

    // Reader-thread-private.
    spatcore::binaural::HeadAttitudePipeline pipeline;
    htk::Stabilizer stabilizer;
};
