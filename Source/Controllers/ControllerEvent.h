#pragma once

/**
 * ControllerEvent — Unified event type for all input controllers.
 *
 * Represents axis movements, button presses/releases, and connection changes
 * from any controller device (SpaceMouse, joystick, gamepad, etc.).
 */

#include <JuceHeader.h>

struct ControllerEvent
{
    enum Type
    {
        AxisMoved,
        ButtonPressed,
        ButtonReleased,
        Connected,
        Disconnected
    };

    Type type;
    int  deviceId      = 0;     // Unique device identifier
    int  axisOrButton  = 0;     // Axis index or button index
    float value        = 0.0f;  // -1..+1 for axes, 1.0 for press, 0.0 for release
    juce::String deviceName;
};
