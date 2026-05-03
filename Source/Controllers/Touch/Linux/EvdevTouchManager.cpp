#if defined (__linux__)

#include "EvdevTouchManager.h"

#include <libudev.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <string.h>

namespace WFSTouch {

static constexpr const char* kSettingsKey = "linuxTouchscreenMappings";

EvdevTouchManager::EvdevTouchManager()
{
    juce::PropertiesFile::Options options;
    options.applicationName     = "WFS-DIY";
    options.filenameSuffix      = ".settings";
    options.osxLibrarySubFolder = "Application Support";
    options.folderName          = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                       .getChildFile ("WFS-DIY").getFullPathName();
    settings = std::make_unique<juce::PropertiesFile> (options);

    udevContext = udev_new();
    if (udevContext == nullptr)
        return;

    loadMappingsFromSettings();
    scanInitial();

    udevMon = udev_monitor_new_from_netlink (udevContext, "udev");
    if (udevMon != nullptr)
    {
        udev_monitor_filter_add_match_subsystem_devtype (udevMon, "input", nullptr);
        udev_monitor_enable_receiving (udevMon);

        if (::pipe2 (udevWakePipe, O_CLOEXEC | O_NONBLOCK) == 0)
        {
            monitorRunning.store (true, std::memory_order_release);
            monitorThread = std::thread ([this] { runUdevMonitor(); });
        }
    }
}

EvdevTouchManager::~EvdevTouchManager()
{
    if (monitorRunning.exchange (false, std::memory_order_acq_rel))
    {
        if (udevWakePipe[1] >= 0) { char b = 1; ::write (udevWakePipe[1], &b, 1); }
        if (monitorThread.joinable()) monitorThread.join();
    }

    if (udevWakePipe[0] >= 0) ::close (udevWakePipe[0]);
    if (udevWakePipe[1] >= 0) ::close (udevWakePipe[1]);

    // Stop devices before tearing down udev (devices don't reference udev directly, but order matters for clarity).
    for (auto& e : entries)
        if (e.device) e.device->stop();
    entries.clear();

    if (udevMon)     udev_monitor_unref (udevMon);
    if (udevContext) udev_unref (udevContext);
}

//==============================================================================
// Discovery
//==============================================================================

void EvdevTouchManager::scanInitial()
{
    if (udevContext == nullptr) return;

    auto* en = udev_enumerate_new (udevContext);
    if (en == nullptr) return;

    udev_enumerate_add_match_subsystem (en, "input");
    udev_enumerate_add_match_property  (en, "ID_INPUT_TOUCHSCREEN", "1");
    udev_enumerate_scan_devices (en);

    for (auto* entry = udev_enumerate_get_list_entry (en);
         entry != nullptr;
         entry = udev_list_entry_get_next (entry))
    {
        const char* path = udev_list_entry_get_name (entry);
        if (path == nullptr) continue;

        auto* dev = udev_device_new_from_syspath (udevContext, path);
        if (dev == nullptr) continue;

        const char* devnode = udev_device_get_devnode (dev);
        if (devnode != nullptr && juce::String (devnode).startsWith ("/dev/input/event"))
        {
            Entry e;
            e.info.sysPath     = path;
            e.info.devNode     = devnode;
            e.info.displayName = describeDevice (path);
            entries.push_back (std::move (e));
        }

        udev_device_unref (dev);
    }

    udev_enumerate_unref (en);

    // Apply any saved mappings; opens devices that were previously enabled.
    for (auto& e : entries)
    {
        auto it = savedMappings.find (e.info.sysPath);
        if (it != savedMappings.end())
            e.info.mapping = it->second;
        applyMapping (e);
    }
}

void EvdevTouchManager::runUdevMonitor()
{
    int monFd = udev_monitor_get_fd (udevMon);

    pollfd pfds[2];
    pfds[0].fd = monFd;          pfds[0].events = POLLIN;
    pfds[1].fd = udevWakePipe[0]; pfds[1].events = POLLIN;

    while (monitorRunning.load (std::memory_order_acquire))
    {
        int rc = ::poll (pfds, 2, -1);
        if (rc < 0)
        {
            if (errno == EINTR) continue;
            break;
        }
        if (pfds[1].revents & POLLIN) break;

        if (pfds[0].revents & POLLIN)
        {
            // Drain all pending udev events; we coalesce into a single rediscover.
            while (auto* dev = udev_monitor_receive_device (udevMon))
            {
                udev_device_unref (dev);
            }
            scheduleRediscover();
        }
    }
}

void EvdevTouchManager::scheduleRediscover()
{
    if (rediscoverPending.exchange (true, std::memory_order_acq_rel))
        return;  // already pending

    juce::WeakReference<EvdevTouchManager> weakSelf (this);
    juce::MessageManager::callAsync ([weakSelf]
    {
        if (auto* self = weakSelf.get())
        {
            self->rediscoverPending.store (false, std::memory_order_release);
            self->rediscoverOnMessageThread();
        }
    });
}

void EvdevTouchManager::rediscoverOnMessageThread()
{
    if (udevContext == nullptr) return;

    auto* en = udev_enumerate_new (udevContext);
    if (en == nullptr) return;
    udev_enumerate_add_match_subsystem (en, "input");
    udev_enumerate_add_match_property  (en, "ID_INPUT_TOUCHSCREEN", "1");
    udev_enumerate_scan_devices (en);

    std::vector<std::pair<juce::String, juce::String>> seen; // sysPath, devNode
    for (auto* le = udev_enumerate_get_list_entry (en); le != nullptr; le = udev_list_entry_get_next (le))
    {
        const char* path = udev_list_entry_get_name (le);
        if (path == nullptr) continue;

        auto* dev = udev_device_new_from_syspath (udevContext, path);
        if (dev != nullptr)
        {
            const char* devnode = udev_device_get_devnode (dev);
            if (devnode != nullptr && juce::String (devnode).startsWith ("/dev/input/event"))
                seen.emplace_back (path, devnode);
            udev_device_unref (dev);
        }
    }
    udev_enumerate_unref (en);

    // Remove entries that are no longer present.
    entries.erase (std::remove_if (entries.begin(), entries.end(),
        [&seen] (const Entry& e)
        {
            for (auto& p : seen) if (p.first == e.info.sysPath) return false;
            return true;
        }), entries.end());

    // Add newcomers.
    for (auto& p : seen)
    {
        bool already = false;
        for (auto& e : entries) if (e.info.sysPath == p.first) { already = true; break; }
        if (already) continue;

        Entry e;
        e.info.sysPath     = p.first;
        e.info.devNode     = p.second;
        e.info.displayName = describeDevice (p.first);
        auto it = savedMappings.find (e.info.sysPath);
        if (it != savedMappings.end())
            e.info.mapping = it->second;
        entries.push_back (std::move (e));
        applyMapping (entries.back());
    }

    fireChange();
}

//==============================================================================
// Mapping & device lifecycle
//==============================================================================

std::vector<EvdevTouchManager::DeviceInfo> EvdevTouchManager::getDetectedDevices() const
{
    std::vector<DeviceInfo> out;
    out.reserve (entries.size());
    for (auto& e : entries) out.push_back (e.info);
    return out;
}

void EvdevTouchManager::setMapping (const juce::String& sysPath, const TouchDeviceMapping& mapping)
{
    savedMappings[sysPath] = mapping;
    saveMappingsToSettings();

    for (auto& e : entries)
    {
        if (e.info.sysPath == sysPath)
        {
            e.info.mapping = mapping;
            applyMapping (e);
            break;
        }
    }

    fireChange();
}

void EvdevTouchManager::applyMapping (Entry& e)
{
    const bool wantOpen = e.info.mapping.isEnabled();

    if (! wantOpen)
    {
        if (e.device)
        {
            e.device->stop();
            e.device.reset();
        }
        e.info.isOpen = false;
        e.info.hasError = false;
        e.info.errorMessage.clear();
        return;
    }

    if (e.device != nullptr)
        return;  // already open

    e.device = std::make_unique<EvdevTouchDevice> (e.info.devNode, e.info.sysPath, e.info.displayName);

    juce::String capturedSysPath = e.info.sysPath;
    juce::WeakReference<EvdevTouchManager> weakSelf (this);
    bool ok = e.device->start ([weakSelf, capturedSysPath] (const EvdevTouchDevice::Snapshot& snap)
    {
        if (auto* self = weakSelf.get())
            self->onSnapshot (capturedSysPath, snap);
    });

    if (! ok)
    {
        e.info.hasError      = true;
        e.info.errorMessage  = e.device->getLastErrorMessage();
        e.info.isOpen        = false;
        e.device.reset();
        return;
    }

    e.info.xRange   = e.device->getXRange();
    e.info.yRange   = e.device->getYRange();
    e.info.isOpen   = true;
    e.info.hasError = false;
    e.info.errorMessage.clear();
}

//==============================================================================
// Snapshot dispatch
//==============================================================================

void EvdevTouchManager::onSnapshot (const juce::String& sysPath, EvdevTouchDevice::Snapshot snap)
{
    juce::WeakReference<EvdevTouchManager> weakSelf (this);
    juce::String spCopy = sysPath;
    juce::MessageManager::callAsync ([weakSelf, spCopy, snap = std::move (snap)]() mutable
    {
        if (auto* self = weakSelf.get())
            self->dispatchSnapshot (spCopy, std::move (snap));
    });
}

void EvdevTouchManager::dispatchSnapshot (const juce::String& sysPath, EvdevTouchDevice::Snapshot snap)
{
    // Find the entry; bail if device removed mid-flight.
    Entry* entry = nullptr;
    for (auto& e : entries) if (e.info.sysPath == sysPath) { entry = &e; break; }
    if (entry == nullptr || ! entry->info.mapping.isEnabled()) return;

    auto& displays = juce::Desktop::getInstance().getDisplays().displays;
    if (entry->info.mapping.displayIndex < 0
        || entry->info.mapping.displayIndex >= displays.size())
        return;

    const auto& disp = displays.getReference (entry->info.mapping.displayIndex);
    auto userArea = disp.userArea;  // logical pixels

    const auto& xR = entry->info.xRange;
    const auto& yR = entry->info.yRange;
    if (! xR.isValid() || ! yR.isValid()) return;

    const auto time = (juce::int64) juce::Time::getMillisecondCounter();
    // JUCE's MouseInputSource::handleEvent inspects the mouse-button bits of
    // the supplied ModifierKeys to decide press / drag / release. For touch we
    // must therefore set leftButtonModifier while the finger is in contact and
    // clear it on the release event — without this every event looks like a
    // hover and the existing touch consumers (MapTab/EQDisplay/PatchMatrix)
    // never see mouseDown/mouseDrag/mouseUp.
    const auto modsDown = juce::ModifierKeys::currentModifiers
                              .withFlags (juce::ModifierKeys::leftButtonModifier);
    const auto modsUp   = juce::ModifierKeys::currentModifiers
                              .withoutFlags (juce::ModifierKeys::leftButtonModifier);

    for (size_t slotIdx = 0; slotIdx < snap.slots.size(); ++slotIdx)
    {
        const auto& s = snap.slots[slotIdx];

        if (! s.pendingDown && ! s.pendingUp && ! s.dirty) continue;

        // Map raw → screen point (logical pixels).
        float nx = (float) (s.rawX - xR.min) / (float) (xR.max - xR.min);
        float ny = (float) (s.rawY - yR.min) / (float) (yR.max - yR.min);
        nx = juce::jlimit (0.0f, 1.0f, nx);
        ny = juce::jlimit (0.0f, 1.0f, ny);

        if (entry->info.mapping.swapXY) std::swap (nx, ny);
        if (entry->info.mapping.flipX)  nx = 1.0f - nx;
        if (entry->info.mapping.flipY)  ny = 1.0f - ny;

        const float screenX = (float) userArea.getX() + nx * (float) userArea.getWidth();
        const float screenY = (float) userArea.getY() + ny * (float) userArea.getHeight();
        const juce::Point<int> screenPt ((int) screenX, (int) screenY);

        FingerKey key { sysPath, (int) slotIdx };

        if (s.pendingDown)
        {
            int idx = acquireTouchIndex (sysPath, (int) slotIdx, s.trackingId);
            if (idx < 0) continue;  // out of indices

            auto* leaf = juce::Desktop::getInstance().findComponentAt (screenPt);
            auto* top  = leaf ? leaf->getTopLevelComponent() : nullptr;
            auto* peer = top  ? top->getPeer() : nullptr;

            FingerState fs;
            fs.touchIndex = idx;
            fs.trackingId = s.trackingId;
            fs.topLevel   = top;
            fs.inContact  = true;
            fingers[key] = fs;

            if (peer != nullptr && top != nullptr)
            {
                auto peerScreenPos = top->getScreenPosition();
                juce::Point<float> peerLocal ((float) (screenPt.x - peerScreenPos.x),
                                              (float) (screenPt.y - peerScreenPos.y));
                peer->handleMouseEvent (juce::MouseInputSource::InputSourceType::touch,
                                        peerLocal, modsDown,
                                        juce::MouseInputSource::defaultPressure,
                                        juce::MouseInputSource::defaultOrientation,
                                        time, {}, idx);
            }
        }
        else
        {
            auto it = fingers.find (key);
            if (it == fingers.end()) continue;

            auto* top  = it->second.topLevel.get();
            auto* peer = top ? top->getPeer() : nullptr;

            if (peer != nullptr && top != nullptr)
            {
                auto peerScreenPos = top->getScreenPosition();
                juce::Point<float> peerLocal ((float) (screenPt.x - peerScreenPos.x),
                                              (float) (screenPt.y - peerScreenPos.y));
                peer->handleMouseEvent (juce::MouseInputSource::InputSourceType::touch,
                                        peerLocal,
                                        s.pendingUp ? modsUp : modsDown,
                                        s.pendingUp ? 0.0f : juce::MouseInputSource::defaultPressure,
                                        juce::MouseInputSource::defaultOrientation,
                                        time, {}, it->second.touchIndex);
            }

            if (s.pendingUp)
            {
                releaseTouchIndex (sysPath, (int) slotIdx);
                fingers.erase (it);
            }
        }
    }
}

//==============================================================================
// Touch index allocation
//==============================================================================

int EvdevTouchManager::acquireTouchIndex (const juce::String& sysPath, int slot, int trackingId)
{
    juce::ignoreUnused (trackingId);
    FingerKey key { sysPath, slot };
    auto it = fingers.find (key);
    if (it != fingers.end()) return it->second.touchIndex;

    for (int i = 0; i <= kMaxTouchIndex; ++i)
    {
        if (! touchIndexInUse[(size_t) i])
        {
            touchIndexInUse[(size_t) i] = true;
            return i;
        }
    }
    return -1;
}

void EvdevTouchManager::releaseTouchIndex (const juce::String& sysPath, int slot)
{
    FingerKey key { sysPath, slot };
    auto it = fingers.find (key);
    if (it == fingers.end()) return;
    int idx = it->second.touchIndex;
    if (idx >= 0 && idx <= kMaxTouchIndex)
        touchIndexInUse[(size_t) idx] = false;
}

int EvdevTouchManager::lookupTouchIndex (const juce::String& sysPath, int slot) const
{
    FingerKey key { sysPath, slot };
    auto it = fingers.find (key);
    return it == fingers.end() ? -1 : it->second.touchIndex;
}

//==============================================================================
// Listener registration
//==============================================================================

int EvdevTouchManager::addChangeListener (ChangeCallback cb)
{
    int token = ++nextListenerToken;
    listeners[token] = std::move (cb);
    return token;
}

void EvdevTouchManager::removeChangeListener (int token)
{
    listeners.erase (token);
}

void EvdevTouchManager::fireChange()
{
    // Copy first so listeners can safely add/remove during dispatch.
    auto copy = listeners;
    for (auto& kv : copy) if (kv.second) kv.second();
}

//==============================================================================
// Persistence
//==============================================================================

void EvdevTouchManager::loadMappingsFromSettings()
{
    if (settings == nullptr) return;
    auto raw = settings->getValue (kSettingsKey, "");
    if (raw.isEmpty()) return;

    auto parsed = juce::JSON::parse (raw);
    if (auto* arr = parsed.getArray())
    {
        for (auto& v : *arr)
        {
            auto m = TouchDeviceMapping::fromVar (v);
            if (m.sysPath.isNotEmpty())
                savedMappings[m.sysPath] = m;
        }
    }
}

void EvdevTouchManager::saveMappingsToSettings()
{
    if (settings == nullptr) return;
    juce::Array<juce::var> arr;
    for (auto& kv : savedMappings)
        arr.add (kv.second.toVar());
    settings->setValue (kSettingsKey, juce::JSON::toString (arr, true));
    settings->saveIfNeeded();
}

//==============================================================================
// Sysfs helpers
//==============================================================================

juce::String EvdevTouchManager::readSysfsAttribute (const juce::String& sysPath, const juce::String& attr)
{
    juce::File f (sysPath + "/" + attr);
    if (! f.existsAsFile())
    {
        // Walk up; many input attributes live in the parent device.
        juce::File parent (sysPath);
        for (int hops = 0; hops < 4 && ! f.existsAsFile(); ++hops)
        {
            parent = parent.getParentDirectory();
            f = parent.getChildFile (attr);
        }
    }
    return f.existsAsFile() ? f.loadFileAsString().trim() : juce::String();
}

juce::String EvdevTouchManager::describeDevice (const juce::String& sysPath)
{
    auto vendor = readSysfsAttribute (sysPath, "device/id/vendor");
    auto product= readSysfsAttribute (sysPath, "device/id/product");
    auto name   = readSysfsAttribute (sysPath, "device/name");
    if (name.isEmpty()) name = readSysfsAttribute (sysPath, "name");

    if (name.isNotEmpty()) return name;
    if (vendor.isNotEmpty() || product.isNotEmpty()) return vendor + ":" + product;
    return juce::File (sysPath).getFileName();
}

} // namespace WFSTouch

#endif // JUCE_LINUX
