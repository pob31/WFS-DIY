#pragma once

#include <JuceHeader.h>
#include "../../spatcore/binaural/HeadOrientationSource.h"
#include "../../spatcore/binaural/HeadFrame.h"
#include "../../spatcore/binaural/HeadAttitudePipeline.h"
#include "headtracker/orientation.hpp"
#include "headtracker/parser.hpp"
#include <atomic>

/**
 * UsbHeadTrackerSource
 *
 * One head unit heard by the USB dongle, as a selectable head-orientation
 * source. The dongle multiplexes every tracker in range onto one serial
 * stream (headtracker PROTOCOL.md); UsbDongleClient demultiplexes by id and
 * hands each tracker's ORIENT packets to its own instance of this class.
 *
 * The publish path mirrors CameraHeadTrackerSource, with one deliberate
 * difference — the side the zero calibration composes on:
 *
 *  - a webcam measures head pose directly, so "set zero" cancels the CAMERA's
 *    placement relative to the stage: a world-side rotation, pre-multiplied;
 *  - this sensor reports body->world, so "set zero" cancels how the puck sits
 *    on the headband: a body-side rotation, post-multiplied.
 *
 * htk::Recenterer implements exactly the second (PROTOCOL.md 1.6). Using the
 * camera's pre-multiply here would be exact only at the capture pose and would
 * conjugate every later rotation into the tilted mounting axes — a true 30 deg
 * head turn reads about 28 deg under a 25 deg mount roll.
 *
 * "Set Zero" maps to boresight() rather than recenter(): a full-pose tare
 * absorbs the mounting angle AND zeroes yaw, so one button covers both setup
 * and the routine drift correction.
 *
 * Threading: the dongle's reader thread is the ONE producer (RtSnapshot
 * contract). Everything it touches is either atomic or reader-thread-private.
 */
class UsbHeadTrackerSource : public spatcore::binaural::SnapshotHeadOrientationSource
{
public:
    explicit UsbHeadTrackerSource (uint16_t trackerId,
                                   const spatcore::binaural::HeadAttitudeTuning& tuning)
        : id (trackerId), pipeline (tuning)
    {
        idString = "usb:" + juce::String::toHexString ((int) trackerId).paddedLeft ('0', 4).toLowerCase();
        // The tracker id is derived from the head unit's factory device ID, so
        // it survives power cycles and is safe to persist in the show file.
        displayName = "Head tracker " + juce::String::toHexString ((int) trackerId)
                                            .paddedLeft ('0', 4).toUpperCase();
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
        // Timestamped, not a plain flag: pressing Set Zero while the dongle is
        // unplugged would otherwise fire minutes later at whatever pose the
        // wearer happened to hold, silently rotating the whole scene. The
        // camera source dodges this only because stop() clears its flag, and
        // this source has no stop.
        auto now = juce::Time::getMillisecondCounter();
        zeroRequestedMs.store (now == 0 ? 1 : now, std::memory_order_release);
    }

    // The dongle is a SHARED device that must already be streaming before any
    // selection can be made (its stream is what the tracker list is built
    // from), so selection does not own its lifetime.
    bool activate() override { return true; }
    void deactivate() override {}

    uint16_t getTrackerId() const noexcept { return id; }

    //==========================================================================
    // Reader thread only.

    void onOrient (const htk_orient& o) noexcept
    {
        lastSampleMs.store (juce::Time::getMillisecondCounter(), std::memory_order_release);

        // quat_of() normalizes and refuses non-finite wire values, but check
        // here too before anything latches: a poisoned Recenterer or filter
        // never recovers, and only this source could repair it.
        const htk::Quat raw = htk::quat_of (o);
        if (! (std::isfinite (raw.w) && std::isfinite (raw.x)
               && std::isfinite (raw.y) && std::isfinite (raw.z)))
        {
            publishOrientation ({});
            return;
        }

        if (const auto requested = zeroRequestedMs.exchange (0, std::memory_order_acq_rel))
        {
            // Drop a stale request rather than applying it to an unrelated
            // pose after a reconnect.
            if (juce::Time::getMillisecondCounter() - requested < kZeroRequestWindowMs)
            {
                recenterer.boresight (raw);
                pipeline.reset();
            }
        }

        const htk::Quat corrected = recenterer.apply (raw);

        float yaw, pitch, roll;
        spatcore::binaural::headframe::trackerQuatToYawPitchRoll (
            corrected.w, corrected.x, corrected.y, corrected.z, yaw, pitch, roll);

        publishOrientation (pipeline.process (yaw, pitch, roll,
                                              juce::Time::getMillisecondCounterHiRes() * 0.001));
    }

    /** The dongle went away. Publish "nothing trustworthy" so the renderer
        slews back to the manual parameters, and drop a pending zero request
        so it cannot fire against a post-reconnect pose. */
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

    // Reader-thread-private.
    htk::Recenterer recenterer;
    spatcore::binaural::HeadAttitudePipeline pipeline;
};
