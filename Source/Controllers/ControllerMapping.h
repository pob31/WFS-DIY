#pragma once

/**
 * ControllerMapping — Axis and button mapping configuration for input controllers.
 *
 * Defines how physical device axes map to WFS actions (position, rotation, etc.)
 * with per-axis sensitivity, dead zone, response curve, and inversion.
 * Includes default profiles for known devices.
 */

#include <JuceHeader.h>

namespace ControllerActions
{
    // Axis target actions
    static const juce::String moveX    = "moveX";
    static const juce::String moveY    = "moveY";
    static const juce::String moveZ    = "moveZ";
    static const juce::String rotate   = "rotate";
    static const juce::String tilt     = "tilt";
    static const juce::String none     = "none";

    // Button actions
    static const juce::String nextInput     = "nextInput";
    static const juce::String prevInput     = "prevInput";
    static const juce::String resetPosition = "resetPosition";
}

//==============================================================================
struct AxisMapping
{
    int          axisIndex   = 0;
    juce::String targetAction = ControllerActions::none;
    float        sensitivity = 1.0f;    // m/s at full deflection (or deg/s for rotation)
    float        deadZone    = 0.05f;   // Ignore values below this threshold
    float        exponent    = 1.0f;    // Response curve: 1.0=linear, 2.0=quadratic
    bool         inverted    = false;

    /** Apply dead zone, exponent curve, sensitivity, and inversion to a raw -1..+1 value.
        Returns the processed value (m/s or deg/s at full deflection). */
    float process (float rawValue) const
    {
        float v = inverted ? -rawValue : rawValue;

        // Dead zone
        float absV = std::abs (v);
        if (absV < deadZone)
            return 0.0f;

        // Rescale after dead zone removal
        float sign = (v >= 0.0f) ? 1.0f : -1.0f;
        float rescaled = (absV - deadZone) / (1.0f - deadZone);

        // Exponent curve
        float curved = std::pow (rescaled, exponent);

        return sign * curved * sensitivity;
    }
};

//==============================================================================
struct ButtonMapping
{
    int          buttonIndex = 0;
    juce::String action = ControllerActions::none;
};

//==============================================================================
struct ControllerProfile
{
    juce::String profileName;
    juce::String devicePattern;   // Match device name by substring (e.g., "SpaceMouse")
    juce::Array<AxisMapping>   axisMappings;
    juce::Array<ButtonMapping> buttonMappings;

    /** Find the mapping for a given axis index, or nullptr if unmapped. */
    const AxisMapping* findAxisMapping (int axisIndex) const
    {
        for (auto& m : axisMappings)
            if (m.axisIndex == axisIndex)
                return &m;
        return nullptr;
    }

    /** Find the mapping for a given button index, or nullptr if unmapped. */
    const ButtonMapping* findButtonMapping (int buttonIndex) const
    {
        for (auto& m : buttonMappings)
            if (m.buttonIndex == buttonIndex)
                return &m;
        return nullptr;
    }

    //==========================================================================
    // Default Profiles
    //==========================================================================

    /** Default profile for 3DConnexion SpaceMouse devices. */
    static ControllerProfile createSpaceMouseDefault()
    {
        ControllerProfile p;
        p.profileName  = "SpaceMouse Default";
        p.devicePattern = "SpaceMouse";

        // Translation axes → position movement
        // SpaceMouse push right = +X → WFS stage right (+X)
        p.axisMappings.add ({ 0, ControllerActions::moveX, 2.0f, 0.05f, 1.0f, false });

        // SpaceMouse push away = +Y → WFS upstage (+Y)
        p.axisMappings.add ({ 1, ControllerActions::moveY, 2.0f, 0.05f, 1.0f, false });

        // SpaceMouse push down = +Z → WFS down (-Z), so invert
        p.axisMappings.add ({ 2, ControllerActions::moveZ, 2.0f, 0.05f, 1.0f, true });

        // Rotation axes — only twist (RotZ) mapped by default
        p.axisMappings.add ({ 3, ControllerActions::none,   0.0f, 0.05f, 1.0f, false });  // RotX unused
        p.axisMappings.add ({ 4, ControllerActions::none,   0.0f, 0.05f, 1.0f, false });  // RotY unused
        p.axisMappings.add ({ 5, ControllerActions::rotate, 90.0f, 0.05f, 1.0f, false }); // RotZ → orientation (deg/s)

        // Buttons: prev/next input
        p.buttonMappings.add ({ 0, ControllerActions::prevInput });
        p.buttonMappings.add ({ 1, ControllerActions::nextInput });

        return p;
    }
};
