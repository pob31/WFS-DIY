#pragma once

#include <JuceHeader.h>
#include "../../spatcore/binaural/HeadOrientationSource.h"
#include "CameraHeadTrackerSource.h"
#include "UsbHeadTrackerSource.h"
#include "UsbDongleClient.h"
#include "../AppSettings.h"
#include <vector>
#include <memory>
#include <atomic>

/**
 * HeadTrackerManager
 *
 * Owns the head-orientation sources for the binaural renderer and hands the
 * active one to the render worker.
 *
 * "Manual orientation" is the absence of an active source (id "manual"):
 * the manual yaw/pitch/roll parameters already reach the worker through the
 * 50 Hz RT snapshot, so nothing needs to be produced for it. Hardware
 * trackers publish through SnapshotHeadOrientationSource from their poll
 * thread — the fast path that makes head motion feel immediate.
 *
 * Device model: a USB receiver dongle (headtracker PROTOCOL.md) that hears
 * every tracker broadcasting in range; each becomes one source with a stable
 * "usb:<id>" id, where the id is the head unit's factory-derived hardware ID.
 * Note the dongle speaks USB CDC-ACM (a virtual serial port), NOT HID — hidapi
 * and the ControllerDevice base do not apply, and htk::Client owns its own
 * reader thread, so wrapping it in one would give two threads per device.
 *
 * Selection vs. device lifetime differ per source, which is what activate()/
 * deactivate() on HeadOrientationSource exist to express:
 *  - the webcam is EXCLUSIVE and runs only while selected;
 *  - the dongle is SHARED and must stream BEFORE any selection, because its
 *    stream is what the tracker list is built from in the first place.
 *
 * Threading:
 *  - enumerate/select run on the message thread.
 *  - `sources` is only ever appended to, and only from the message thread
 *    (tracker discovery happens on a reader thread and must marshal first).
 *    Elements are unique_ptr, so pointees keep stable addresses across any
 *    reallocation and the render worker's raw pointer stays valid.
 *  - Sources are never removed. A tracker that goes silent stays listed with
 *    isConnected() == false, so the selected source object survives an unplug
 *    and simply resumes when the device returns.
 *  - The render worker reads the active source through a raw pointer set
 *    with an atomic (sources are destroyed after the worker stops, so a stale
 *    read is at worst one block behind).
 */
class HeadTrackerManager
{
public:
    struct SourceInfo
    {
        juce::String id;            // stable, persisted ("manual", "usb:<serial>")
        juce::String displayName;
        bool connected = false;
    };

    HeadTrackerManager()
    {
        // Webcam tracking via the wfs_headtrack plugin DLL. Always listed:
        // whether the plugin/camera actually works is only knowable by trying,
        // and a failed selection logs its reason and falls back to manual.
        sources.push_back (std::make_unique<CameraHeadTrackerSource>());

        // The dongle starts streaming immediately, before anything is
        // selected: its stream is what the tracker list is built from.
        dongle = std::make_unique<UsbDongleClient> ([this] (uint16_t id)
        {
            return addUsbTracker (id);
        });
        dongle->start();
    }

    ~HeadTrackerManager()
    {
        shutdown();
    }

    /** Stops the dongle's reader thread and joins it. MUST complete before
        `sources` is destroyed — that thread calls into the tracker sources.
        Declaration order alone would do it (dongle is declared after sources,
        so it dies first), but this is too sharp an edge to leave implicit. */
    void shutdown()
    {
        if (dongle != nullptr)
            dongle->shutdown();
    }

    /** All selectable sources, "Manual orientation" first. Message thread. */
    std::vector<SourceInfo> enumerateSources() const
    {
        std::vector<SourceInfo> list;
        list.push_back ({ "manual", "Manual orientation", true });
        for (const auto& s : sources)
            list.push_back ({ s->getSourceId(), s->getDisplayName(), s->isConnected() });
        return list;
    }

    /** Select by stable id. Unknown ids (device not present) fall back to
        manual — the persisted id is kept by the caller so the device can be
        re-selected when it reappears. Message thread. */
    void setActiveSource (const juce::String& sourceId)
    {
        spatcore::binaural::HeadOrientationSource* next = nullptr;
        for (const auto& s : sources)
            if (s->getSourceId() == sourceId)
                next = s.get();

        // Release before acquiring, so two sources sharing one exclusive
        // device (two entries for the same webcam, say) can never both hold it
        // during a swap. activate() failing → manual fallback, with the reason
        // logged by the source itself. Sources whose device is shared and
        // always-on (the USB dongle) implement both as no-ops.
        for (const auto& s : sources)
            if (s.get() != next)
                s->deactivate();

        if (next != nullptr && ! next->activate())
            next = nullptr;

        activeSource.store (next, std::memory_order_release);
    }

    /** "Set zero while looking at the stage" for whatever tracker is active.
        No-op in manual mode. Message thread. */
    void setZeroOnActiveSource()
    {
        if (auto* source = getActiveSource())
            source->setZero();
    }

    /** Render worker: nullptr = manual orientation. */
    spatcore::binaural::HeadOrientationSource* getActiveSource() const noexcept
    {
        return activeSource.load (std::memory_order_acquire);
    }

    /** Is `sourceId` a source we know about? Message thread. Used to re-bind a
        persisted selection once its device turns up. */
    bool hasSource (const juce::String& sourceId) const
    {
        for (const auto& s : sources)
            if (s->getSourceId() == sourceId)
                return true;
        return false;
    }

private:
    /** Message thread, called by the dongle's discovery timer. Appending here
        is what keeps the lock-free contract honest: the render worker holds a
        raw pointer into `sources`, and unique_ptr elements keep stable
        addresses across any reallocation. */
    UsbHeadTrackerSource* addUsbTracker (uint16_t id)
    {
        JUCE_ASSERT_MESSAGE_THREAD

        spatcore::binaural::HeadAttitudeTuning tuning;
        const auto hz = (float) AppSettings::getHeadtrackUsbSmoothingHz();
        tuning.minCutoffHz = hz;
        // beta multiplies rad/s. A VQF-fused IMU at 208 Hz is far quieter than
        // 60 fps face landmarks, so it needs neither the camera's 1.5 Hz floor
        // (~106 ms of group delay at rest — which would make the hardware
        // tracker feel WORSE than the webcam it replaces) nor its aggressive
        // speed term.
        // TODO(hardware): re-tune against a real head unit at bring-up.
        tuning.beta = 1.0f;
        tuning.derivCutoffHz = 1.0f;

        auto source = std::make_unique<UsbHeadTrackerSource> (id, tuning);
        auto* raw = source.get();
        sources.push_back (std::move (source));
        return raw;
    }

    std::vector<std::unique_ptr<spatcore::binaural::HeadOrientationSource>> sources;
    std::atomic<spatcore::binaural::HeadOrientationSource*> activeSource { nullptr };

    // Declared AFTER `sources` on purpose: destruction runs in reverse, so the
    // reader thread is joined before the objects it calls into are destroyed.
    std::unique_ptr<UsbDongleClient> dongle;
};
