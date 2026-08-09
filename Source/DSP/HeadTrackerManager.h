#pragma once

#include <JuceHeader.h>
#include "../../spatcore/binaural/HeadOrientationSource.h"
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
 * Device model (future): a USB receiver dongle that enumerates every tracker
 * it can hear; each becomes one source with a stable "usb:<serial>" id. The
 * driver will follow the spatcore/controllers/ControllerDevice.h pattern
 * (juce::Thread poll loop + 2 s hotplug Timer, hidapi like
 * spatcore/controllers/spacemouse/SpaceMouseDevice.h — hidapi is already
 * vendored). The receiver protocol is TBD; UsbHeadTrackerDevice below is the
 * intended shape, kept dormant until the protocol lands.
 *
 * Threading:
 *  - enumerate/select run on the message thread.
 *  - The render worker reads the active source through a raw pointer set
 *    with an atomic (sources are only ever added, and destroyed after the
 *    worker stops, so a stale read is at worst one block behind).
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

    HeadTrackerManager() = default;

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
        activeSource.store (next, std::memory_order_release);
    }

    /** Render worker: nullptr = manual orientation. */
    spatcore::binaural::HeadOrientationSource* getActiveSource() const noexcept
    {
        return activeSource.load (std::memory_order_acquire);
    }

private:
    /**
     * Skeleton for the USB head-tracker receiver driver. Dormant: nothing
     * instantiates it until the receiver protocol is defined.
     *
     * Intended shape once the protocol lands:
     *   - inherit spatcore::ControllerDevice for the poll thread + hotplug
     *     rescan (2 s), hidapi enumeration by the receiver's VID/PID;
     *   - the receiver reports every tracker it hears; this driver creates
     *     one SnapshotHeadOrientationSource per tracker keyed "usb:<serial>";
     *   - per report: quaternion (or yaw/pitch/roll) → publishOrientation()
     *     with valid=true; a staleness timeout publishes valid=false so the
     *     renderer slews back to manual;
     *   - "set zero while looking at the stage" calibration stores the
     *     inverse of the current attitude and pre-multiplies every report.
     */
    class UsbHeadTrackerDevice; // TODO(protocol): implement per the note above

    std::vector<std::unique_ptr<spatcore::binaural::HeadOrientationSource>> sources;
    std::atomic<spatcore::binaural::HeadOrientationSource*> activeSource { nullptr };
};
