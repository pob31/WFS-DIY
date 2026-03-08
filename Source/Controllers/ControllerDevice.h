#pragma once

/**
 * ControllerDevice — Abstract base class for input controller devices.
 *
 * Provides the common Thread + Timer pattern used by all controller drivers:
 *   - Background thread for polling device input (run/poll override)
 *   - Timer for hotplug detection (tryConnect/checkConnection)
 *   - Events dispatched to GUI thread via MessageManager::callAsync
 *
 * Subclasses implement: tryConnect(), poll(), disconnect(), and device metadata.
 */

#include <JuceHeader.h>
#include "ControllerEvent.h"

class ControllerDevice : private juce::Thread,
                         private juce::Timer
{
public:
    //==========================================================================
    // Callbacks (set by owner, called on GUI thread)
    //==========================================================================

    std::function<void (const ControllerEvent&)> onEvent;

    //==========================================================================
    // Construction / Destruction
    //==========================================================================

    explicit ControllerDevice (const juce::String& threadName)
        : juce::Thread (threadName),
          alive (std::make_shared<bool> (true))
    {
    }

    ~ControllerDevice() override
    {
        // Mark as dead so pending callAsync lambdas won't call into destroyed object
        *alive = false;

        // Only stop thread/timer here — do NOT call disconnect() (virtual)
        // since the derived class is already destroyed at this point.
        // Derived classes must call stopMonitoring() in their own destructor.
        stopTimer();
        signalThreadShouldExit();
        if (isThreadRunning())
            stopThread (500);
    }

    //==========================================================================
    // Lifecycle
    //==========================================================================

    /** Start monitoring for device connection. Call once at app startup. */
    void startMonitoring()
    {
        startTimer (2000);  // Check for device every 2 seconds
        tryConnect();
    }

    /** Stop monitoring and disconnect. Call at app shutdown. */
    void stopMonitoring()
    {
        stopTimer();
        signalThreadShouldExit();

        if (isThreadRunning())
            stopThread (500);

        disconnect();
    }

    //==========================================================================
    // Device info (override in subclasses)
    //==========================================================================

    virtual bool isConnected() const = 0;
    virtual juce::String getDeviceName() const = 0;
    virtual int getDeviceId() const = 0;

    virtual int getNumAxes() const = 0;
    virtual int getNumButtons() const = 0;
    virtual juce::StringArray getAxisNames() const = 0;
    virtual juce::StringArray getButtonNames() const = 0;

protected:
    //==========================================================================
    // Subclass interface
    //==========================================================================

    /** Attempt to connect to the device. Return true if connection succeeded. */
    virtual bool tryConnect() = 0;

    /** Disconnect and release device resources. */
    virtual void disconnect() = 0;

    /** Poll the device for input. Called repeatedly from the background thread.
        Should call dispatchEvent() for any axis/button changes detected.
        Return false to exit the polling loop (e.g., device disconnected). */
    virtual bool poll() = 0;

    /** Dispatch an event to the GUI thread. Safe to call from any thread.
        Uses a shared alive flag so pending callbacks are safely ignored after destruction. */
    void dispatchEvent (ControllerEvent event)
    {
        auto aliveFlag = alive;
        auto callback  = onEvent;  // Copy the std::function (safe to capture)
        juce::MessageManager::callAsync ([aliveFlag, callback, e = std::move (event)]()
        {
            if (*aliveFlag && callback)
                callback (e);
        });
    }

private:
    //==========================================================================
    // juce::Thread — background polling loop
    //==========================================================================

    void run() override
    {
        while (! threadShouldExit())
        {
            if (! poll())
                break;
        }

        // Device lost during polling — notify and clean up
        if (! threadShouldExit() && isConnected())
        {
            disconnect();
            dispatchEvent ({ ControllerEvent::Disconnected, getDeviceId(), 0, 0.0f, getDeviceName() });
        }
    }

    //==========================================================================
    // juce::Timer — hotplug detection
    //==========================================================================

    void timerCallback() override
    {
        if (! isConnected())
        {
            if (tryConnect())
            {
                startThread (juce::Thread::Priority::normal);
                dispatchEvent ({ ControllerEvent::Connected, getDeviceId(), 0, 0.0f, getDeviceName() });
            }
        }
    }

    std::shared_ptr<bool> alive;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControllerDevice)
};
