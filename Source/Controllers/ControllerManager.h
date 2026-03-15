#pragma once

/**
 * ControllerManager — Central orchestrator for input controller devices.
 *
 * Owns all ControllerDevice instances, receives their events on the GUI thread,
 * applies axis mappings (dead zone, sensitivity, exponent curve), and performs
 * velocity integration at 50 Hz to produce smooth position deltas.
 *
 * Owned by MainComponent, wired to MapTab via callbacks.
 */

#include <JuceHeader.h>
#include "ControllerEvent.h"
#include "ControllerDevice.h"
#include "ControllerMapping.h"

class ControllerManager : private juce::Timer
{
public:
    //==========================================================================
    // Callbacks (set by MainComponent to wire into MapTab / InputsTab)
    //==========================================================================

    struct Callbacks
    {
        /** Move all currently selected inputs by a delta in meters. */
        std::function<void (float dx, float dy, float dz)> moveSelectedDelta;

        /** Rotate selected input orientation by deltaDeg degrees. */
        std::function<void (float deltaDeg)> rotateSelected;

        /** Cycle input selection: +1 = next, -1 = prev. */
        std::function<void (int delta)> cycleInput;

        /** Get number of inputs for cycling bounds. */
        std::function<int()> getNumInputs;

        /** Get the set of currently selected input indices (for rotation). */
        std::function<std::set<int>()> getSelectedInputs;

        /** Trigger map repaint after position changes. */
        std::function<void()> repaintMap;

        // Cluster callbacks (used when activeTab == 5)
        std::function<void (float dx, float dy, float dz)> moveClusterDelta;
        std::function<void (float deltaDeg)> rotateCluster;
        std::function<void (float scaleFactor)> scaleCluster;  // multiplicative, 1.0 = no change
    };

    Callbacks callbacks;

    /** Active tab index — set by MainComponent on tab change.
        When 5 (Clusters), SpaceMouse controls the selected cluster. */
    int activeTab = -1;

    //==========================================================================
    // Construction / Destruction
    //==========================================================================

    ControllerManager()
    {
    }

    ~ControllerManager() override
    {
        stopTimer();
        for (auto& device : devices)
            device->stopMonitoring();
    }

    //==========================================================================
    // Device Management
    //==========================================================================

    /** Add a controller device and start monitoring it.
        The manager takes ownership and wires the event callback. */
    void addDevice (std::unique_ptr<ControllerDevice> device)
    {
        device->onEvent = [this] (const ControllerEvent& e) { handleEvent (e); };
        device->startMonitoring();
        devices.push_back (std::move (device));
    }

    /** Get the number of registered devices. */
    int getNumDevices() const { return static_cast<int> (devices.size()); }

    /** Get a device by index. */
    ControllerDevice* getDevice (int index) const
    {
        if (index >= 0 && index < static_cast<int> (devices.size()))
            return devices[static_cast<size_t> (index)].get();
        return nullptr;
    }

    //==========================================================================
    // Profile Management
    //==========================================================================

    /** Set the active profile for a given device ID. */
    void setProfile (int deviceId, const ControllerProfile& profile)
    {
        profiles[deviceId] = profile;
    }

    /** Get the active profile for a device ID, or nullptr if none. */
    const ControllerProfile* getProfile (int deviceId) const
    {
        auto it = profiles.find (deviceId);
        return (it != profiles.end()) ? &it->second : nullptr;
    }

    //==========================================================================
    // Enable / Disable
    //==========================================================================

    void setEnabled (bool shouldBeEnabled)
    {
        enabled = shouldBeEnabled;

        if (enabled && ! isTimerRunning())
            startTimer (20);  // 50 Hz velocity integration
        else if (! enabled && isTimerRunning())
            stopTimer();
    }

    bool isEnabled() const { return enabled; }

    /** Start the velocity integration timer. Call after setting callbacks. */
    void start()
    {
        enabled = true;
        startTimer (20);  // 50 Hz
    }

private:
    //==========================================================================
    // Event Handling (called on GUI thread via device's callAsync)
    //==========================================================================

    void handleEvent (const ControllerEvent& event)
    {
        if (! enabled)
            return;

        switch (event.type)
        {
            case ControllerEvent::AxisMoved:
                handleAxisEvent (event);
                break;

            case ControllerEvent::ButtonPressed:
            case ControllerEvent::ButtonReleased:
                handleButtonEvent (event);
                break;

            case ControllerEvent::Connected:
                handleConnection (event);
                break;

            case ControllerEvent::Disconnected:
                DBG ("Controller disconnected: " + event.deviceName);
                break;

            default:
                break;
        }
    }

    void handleAxisEvent (const ControllerEvent& event)
    {
        // Cache the latest raw axis value — velocity integration happens in timerCallback
        auto key = std::make_pair (event.deviceId, event.axisOrButton);
        latestAxisValues[key] = event.value;
    }

    void handleButtonEvent (const ControllerEvent& event)
    {
        // Track persistent button state (for modifier behavior)
        buttonStates[{ event.deviceId, event.axisOrButton }] =
            (event.type == ControllerEvent::ButtonPressed);

        // On Clusters tab, buttons are modifiers only — don't fire actions
        if (activeTab == 5)
            return;

        const auto* profile = getProfile (event.deviceId);
        if (profile == nullptr)
            return;

        const auto* mapping = profile->findButtonMapping (event.axisOrButton);
        if (mapping == nullptr)
            return;

        if (mapping->action == ControllerActions::nextInput && callbacks.cycleInput)
            callbacks.cycleInput (+1);
        else if (mapping->action == ControllerActions::prevInput && callbacks.cycleInput)
            callbacks.cycleInput (-1);
    }

    void handleConnection (const ControllerEvent& event)
    {
        DBG ("Controller connected: " + event.deviceName);

        // Auto-assign a default profile if none exists for this device
        if (profiles.find (event.deviceId) == profiles.end())
        {
            if (event.deviceName.containsIgnoreCase ("SpaceMouse")
                || event.deviceName.containsIgnoreCase ("3Dconnexion"))
            {
                setProfile (event.deviceId, ControllerProfile::createSpaceMouseDefault());
                DBG ("  -> Applied default SpaceMouse profile");
            }
        }
    }

    //==========================================================================
    // Timer: Velocity Integration (50 Hz)
    //==========================================================================

    void timerCallback() override
    {
        if (! enabled)
            return;

        constexpr float dt = 0.02f;  // 50 Hz → 20ms per tick

        float totalDx = 0.0f, totalDy = 0.0f, totalDz = 0.0f;
        float totalRotation = 0.0f;

        // Process all cached axis values through their mappings
        for (auto& [key, rawValue] : latestAxisValues)
        {
            auto [deviceId, axisIndex] = key;
            const auto* profile = getProfile (deviceId);
            if (profile == nullptr)
                continue;

            const auto* mapping = profile->findAxisMapping (axisIndex);
            if (mapping == nullptr || mapping->targetAction == ControllerActions::none)
                continue;

            float velocity = mapping->process (rawValue);  // m/s or deg/s
            if (std::abs (velocity) < 0.001f)
                continue;

            float delta = velocity * dt;

            if (mapping->targetAction == ControllerActions::moveX)
                totalDx += delta;
            else if (mapping->targetAction == ControllerActions::moveY)
                totalDy += delta;
            else if (mapping->targetAction == ControllerActions::moveZ)
                totalDz += delta;
            else if (mapping->targetAction == ControllerActions::rotate)
                totalRotation += delta;
        }

        if (activeTab == 5)
        {
            // Clusters tab: TransZ switches between height and scale based on button state
            float moveZ = isAnyButtonPressed() ? 0.0f : totalDz;
            float scaleDelta = isAnyButtonPressed() ? totalDz : 0.0f;

            if (callbacks.moveClusterDelta
                && (std::abs (totalDx) > 0.0001f || std::abs (totalDy) > 0.0001f || std::abs (moveZ) > 0.0001f))
            {
                callbacks.moveClusterDelta (totalDx, totalDy, moveZ);
            }

            if (callbacks.rotateCluster && std::abs (totalRotation) > 0.01f)
                callbacks.rotateCluster (totalRotation);

            if (callbacks.scaleCluster && std::abs (scaleDelta) > 0.001f)
            {
                // Convert Z velocity delta (meters) to multiplicative scale factor
                float scaleFactor = 1.0f + scaleDelta * 0.5f;
                callbacks.scaleCluster (juce::jlimit (0.95f, 1.05f, scaleFactor));
            }
        }
        else
        {
            // Default: move selected inputs + rotate
            if ((std::abs (totalDx) > 0.0001f || std::abs (totalDy) > 0.0001f || std::abs (totalDz) > 0.0001f)
                && callbacks.moveSelectedDelta)
            {
                callbacks.moveSelectedDelta (totalDx, totalDy, totalDz);
            }

            if (std::abs (totalRotation) > 0.01f && callbacks.rotateSelected && callbacks.getSelectedInputs)
            {
                auto selected = callbacks.getSelectedInputs();
                if (! selected.empty())
                    callbacks.rotateSelected (totalRotation);
            }
        }
    }

    //==========================================================================
    // State
    //==========================================================================

    std::vector<std::unique_ptr<ControllerDevice>> devices;
    std::map<int, ControllerProfile> profiles;

    // Latest axis values keyed by (deviceId, axisIndex), updated from events,
    // consumed by timerCallback for velocity integration.
    std::map<std::pair<int, int>, float> latestAxisValues;

    // Persistent button state for modifier behavior (e.g. held = scale mode)
    std::map<std::pair<int, int>, bool> buttonStates;

    bool isAnyButtonPressed() const
    {
        for (auto& [key, pressed] : buttonStates)
            if (pressed) return true;
        return false;
    }

    bool enabled = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControllerManager)
};
