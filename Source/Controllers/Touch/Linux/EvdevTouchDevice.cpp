#if defined (__linux__)

#include "EvdevTouchDevice.h"

#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <linux/input.h>
#include <sys/ioctl.h>

namespace WFSTouch {

EvdevTouchDevice::EvdevTouchDevice (const juce::String& devNodePath,
                                    const juce::String& sysPathIn,
                                    const juce::String& displayNameIn)
    : devNode (devNodePath), sysPath (sysPathIn), displayName (displayNameIn)
{
}

EvdevTouchDevice::~EvdevTouchDevice()
{
    stop();
}

bool EvdevTouchDevice::start (SnapshotCallback cb)
{
    if (running.load (std::memory_order_acquire))
        return true;

    snapshotCallback = std::move (cb);

    fd = ::open (devNode.toRawUTF8(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0)
    {
        lastError = "open(" + devNode + ") failed: " + juce::String (strerror (errno));
        return false;
    }

    input_absinfo absX{}, absY{};
    if (::ioctl (fd, EVIOCGABS (ABS_MT_POSITION_X), &absX) >= 0)
    {
        xRange.min = absX.minimum;
        xRange.max = absX.maximum;
    }
    if (::ioctl (fd, EVIOCGABS (ABS_MT_POSITION_Y), &absY) >= 0)
    {
        yRange.min = absY.minimum;
        yRange.max = absY.maximum;
    }

    if (! xRange.isValid() || ! yRange.isValid())
    {
        lastError = "device does not advertise ABS_MT_POSITION_{X,Y}";
        ::close (fd); fd = -1;
        return false;
    }

    if (::ioctl (fd, EVIOCGRAB, (void*) 1) < 0)
    {
        lastError = "EVIOCGRAB failed: " + juce::String (strerror (errno));
        ::close (fd); fd = -1;
        return false;
    }

    if (::pipe2 (wakePipe, O_CLOEXEC | O_NONBLOCK) < 0)
    {
        lastError = "pipe2 failed: " + juce::String (strerror (errno));
        ::ioctl (fd, EVIOCGRAB, (void*) 0);
        ::close (fd); fd = -1;
        return false;
    }

    running.store (true, std::memory_order_release);
    worker = std::thread ([this] { runReadLoop(); });
    lastError.clear();
    return true;
}

void EvdevTouchDevice::stop()
{
    if (! running.exchange (false, std::memory_order_acq_rel))
        return;

    if (wakePipe[1] >= 0)
    {
        char b = 1;
        ::write (wakePipe[1], &b, 1);
    }

    if (worker.joinable())
        worker.join();

    if (wakePipe[0] >= 0) { ::close (wakePipe[0]); wakePipe[0] = -1; }
    if (wakePipe[1] >= 0) { ::close (wakePipe[1]); wakePipe[1] = -1; }

    if (fd >= 0)
    {
        ::ioctl (fd, EVIOCGRAB, (void*) 0);
        ::close (fd);
        fd = -1;
    }
}

void EvdevTouchDevice::runReadLoop()
{
    Snapshot state;
    int currentSlot = 0;

    pollfd pfds[2];
    pfds[0].fd     = fd;
    pfds[0].events = POLLIN;
    pfds[1].fd     = wakePipe[0];
    pfds[1].events = POLLIN;

    while (running.load (std::memory_order_acquire))
    {
        int rc = ::poll (pfds, 2, -1);
        if (rc < 0)
        {
            if (errno == EINTR) continue;
            break;
        }

        if (pfds[1].revents & POLLIN)
            break;  // wake pipe → shutdown

        if (! (pfds[0].revents & POLLIN))
            continue;

        input_event events[64];
        ssize_t n = ::read (fd, events, sizeof (events));
        if (n <= 0)
        {
            if (errno == EAGAIN || errno == EINTR) continue;
            break;  // device unplugged or unrecoverable
        }

        const size_t count = (size_t) n / sizeof (input_event);
        bool snapshotReady = false;

        for (size_t i = 0; i < count; ++i)
        {
            const auto& ev = events[i];

            if (ev.type == EV_SYN && ev.code == SYN_REPORT)
            {
                snapshotReady = true;
                continue;
            }

            if (ev.type == EV_ABS)
            {
                switch (ev.code)
                {
                    case ABS_MT_SLOT:
                        if (ev.value >= 0 && ev.value < kMaxSlots)
                            currentSlot = ev.value;
                        break;

                    case ABS_MT_TRACKING_ID:
                    {
                        auto& s = state.slots[(size_t) currentSlot];
                        if (ev.value < 0)
                        {
                            if (s.active)
                            {
                                s.pendingUp = true;
                                s.active = false;
                                s.trackingId = -1;
                            }
                        }
                        else
                        {
                            s.trackingId = ev.value;
                            s.active = true;
                            s.pendingDown = true;
                        }
                        break;
                    }

                    case ABS_MT_POSITION_X:
                    {
                        auto& s = state.slots[(size_t) currentSlot];
                        if (s.rawX != ev.value) { s.rawX = ev.value; s.dirty = true; }
                        break;
                    }

                    case ABS_MT_POSITION_Y:
                    {
                        auto& s = state.slots[(size_t) currentSlot];
                        if (s.rawY != ev.value) { s.rawY = ev.value; s.dirty = true; }
                        break;
                    }

                    default:
                        break;
                }
            }
        }

        if (snapshotReady && snapshotCallback)
        {
            // Copy the snapshot then clear edge flags before the next frame.
            Snapshot toSend = state;
            for (auto& s : state.slots)
            {
                s.pendingDown = false;
                s.pendingUp   = false;
                s.dirty       = false;
            }
            snapshotCallback (toSend);
        }
    }

    running.store (false, std::memory_order_release);
}

} // namespace WFSTouch

#endif // JUCE_LINUX
