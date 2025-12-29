#pragma once

#include <JuceHeader.h>
#include <cmath>

/**
 * Coordinate conversion utilities for WFS position display/input.
 *
 * Supports three coordinate systems:
 * - Cartesian (X, Y, Z) in meters - internal storage format
 * - Cylindrical (r, theta, Z) - radius in meters, azimuth in degrees, height in meters
 * - Spherical (r, theta, phi) - radius in meters, azimuth and elevation in degrees
 *
 * Angle Conventions (WFS Stage Coordinate System):
 * - Azimuth (theta): 0 deg = toward audience (-Y in Cartesian)
 *                    180/-180 deg = upstage (+Y)
 *                    90 deg = stage right (+X)
 *                    -90 deg = stage left (-X)
 * - Elevation (phi): 0 deg = horizontal plane
 *                    90 deg = up (+Z)
 *                    -90 deg = down (-Z)
 */
namespace WFSCoordinates
{
    //==========================================================================
    // Coordinate Mode Enum
    //==========================================================================

    enum class Mode
    {
        Cartesian = 0,    // X, Y, Z (meters)
        Cylindrical = 1,  // r, theta, Z (meters, degrees, meters)
        Spherical = 2     // r, theta, phi (meters, degrees, degrees)
    };

    //==========================================================================
    // Coordinate Structs
    //==========================================================================

    struct CartesianCoord
    {
        float x;  // meters, positive = stage right
        float y;  // meters, positive = upstage (back)
        float z;  // meters, positive = up
    };

    struct CylindricalCoord
    {
        float r;      // radius in meters (>= 0)
        float theta;  // azimuth in degrees (-180 to 180, 0 = audience)
        float z;      // height in meters
    };

    struct SphericalCoord
    {
        float r;      // radius in meters (>= 0)
        float theta;  // azimuth in degrees (-180 to 180, 0 = audience)
        float phi;    // elevation in degrees (-90 to 90, 0 = horizontal)
    };

    //==========================================================================
    // Utility Functions
    //==========================================================================

    /** Normalize angle to -180 to 180 degrees range */
    inline float normalizeAngle(float degrees)
    {
        while (degrees > 180.0f)
            degrees -= 360.0f;
        while (degrees <= -180.0f)
            degrees += 360.0f;
        return degrees;
    }

    /** Clamp elevation to -90 to 90 degrees range */
    inline float clampElevation(float degrees)
    {
        return juce::jlimit(-90.0f, 90.0f, degrees);
    }

    //==========================================================================
    // Cartesian <-> Cylindrical Conversions
    //==========================================================================

    /**
     * Convert Cartesian to Cylindrical coordinates.
     * Uses WFS convention: theta=0 toward audience (-Y), theta=90 stage right (+X)
     */
    inline CylindricalCoord cartesianToCylindrical(const CartesianCoord& c)
    {
        float r = std::sqrt(c.x * c.x + c.y * c.y);

        // atan2(-y, x) gives angle where 0 = toward -Y (audience)
        // Then adjust so theta=90 is +X (stage right)
        float theta = 0.0f;
        if (r > 0.0001f)
        {
            // atan2(x, -y) gives: 0 when pointing to -Y, 90 when pointing to +X
            theta = juce::radiansToDegrees(std::atan2(c.x, -c.y));
        }

        return { r, normalizeAngle(theta), c.z };
    }

    /**
     * Convert Cylindrical to Cartesian coordinates.
     * Uses WFS convention: theta=0 toward audience (-Y), theta=90 stage right (+X)
     */
    inline CartesianCoord cylindricalToCartesian(const CylindricalCoord& cyl)
    {
        float thetaRad = juce::degreesToRadians(cyl.theta);

        // theta=0 -> -Y (audience), theta=90 -> +X (stage right)
        // x = r * sin(theta), y = -r * cos(theta)
        float x = cyl.r * std::sin(thetaRad);
        float y = -cyl.r * std::cos(thetaRad);

        return { x, y, cyl.z };
    }

    //==========================================================================
    // Cartesian <-> Spherical Conversions
    //==========================================================================

    /**
     * Convert Cartesian to Spherical coordinates.
     * Uses WFS convention: theta=0 toward audience (-Y), phi=0 horizontal
     */
    inline SphericalCoord cartesianToSpherical(const CartesianCoord& c)
    {
        float r = std::sqrt(c.x * c.x + c.y * c.y + c.z * c.z);

        float theta = 0.0f;
        float phi = 0.0f;

        if (r > 0.0001f)
        {
            // Elevation: asin(z/r), 0 = horizontal, 90 = up
            phi = juce::radiansToDegrees(std::asin(c.z / r));

            // Azimuth: same as cylindrical
            float rHoriz = std::sqrt(c.x * c.x + c.y * c.y);
            if (rHoriz > 0.0001f)
            {
                theta = juce::radiansToDegrees(std::atan2(c.x, -c.y));
            }
        }

        return { r, normalizeAngle(theta), clampElevation(phi) };
    }

    /**
     * Convert Spherical to Cartesian coordinates.
     * Uses WFS convention: theta=0 toward audience (-Y), phi=0 horizontal
     */
    inline CartesianCoord sphericalToCartesian(const SphericalCoord& sph)
    {
        float thetaRad = juce::degreesToRadians(sph.theta);
        float phiRad = juce::degreesToRadians(sph.phi);

        // Horizontal radius projection
        float rHoriz = sph.r * std::cos(phiRad);

        // x = rHoriz * sin(theta), y = -rHoriz * cos(theta), z = r * sin(phi)
        float x = rHoriz * std::sin(thetaRad);
        float y = -rHoriz * std::cos(thetaRad);
        float z = sph.r * std::sin(phiRad);

        return { x, y, z };
    }

    //==========================================================================
    // Display Formatting
    //==========================================================================

    /**
     * Format coordinate values for display based on mode.
     * Takes Cartesian input (x, y, z) and formats according to selected mode.
     */
    inline juce::String formatCoordinate(Mode mode, float x, float y, float z)
    {
        switch (mode)
        {
            case Mode::Cylindrical:
            {
                auto cyl = cartesianToCylindrical({ x, y, z });
                return "r=" + juce::String(cyl.r, 1) + "m "
                     + juce::String(juce::CharPointer_UTF8("\xce\xb8")) + "="
                     + juce::String(cyl.theta, 0) + juce::String(juce::CharPointer_UTF8("\xc2\xb0"));
            }

            case Mode::Spherical:
            {
                auto sph = cartesianToSpherical({ x, y, z });
                return "r=" + juce::String(sph.r, 1) + "m "
                     + juce::String(juce::CharPointer_UTF8("\xce\xb8")) + "="
                     + juce::String(sph.theta, 0) + juce::String(juce::CharPointer_UTF8("\xc2\xb0")) + " "
                     + juce::String(juce::CharPointer_UTF8("\xcf\x86")) + "="
                     + juce::String(sph.phi, 0) + juce::String(juce::CharPointer_UTF8("\xc2\xb0"));
            }

            default: // Cartesian
                return "(" + juce::String(x, 1) + ", " + juce::String(y, 1) + ")";
        }
    }

    /**
     * Get label and unit strings for coordinate mode.
     * Updates the passed references with appropriate labels and units.
     */
    inline void getCoordinateLabels(Mode mode,
                                    juce::String& label1, juce::String& label2, juce::String& label3,
                                    juce::String& unit1, juce::String& unit2, juce::String& unit3)
    {
        switch (mode)
        {
            case Mode::Cylindrical:
                label1 = "Radius:";
                label2 = "Azimuth:";
                label3 = "Height:";
                unit1 = "m";
                unit2 = juce::String(juce::CharPointer_UTF8("\xc2\xb0"));  // degree symbol
                unit3 = "m";
                break;

            case Mode::Spherical:
                label1 = "Radius:";
                label2 = "Azimuth:";
                label3 = "Elevation:";
                unit1 = "m";
                unit2 = juce::String(juce::CharPointer_UTF8("\xc2\xb0"));
                unit3 = juce::String(juce::CharPointer_UTF8("\xc2\xb0"));
                break;

            default: // Cartesian
                label1 = "Position X:";
                label2 = "Position Y:";
                label3 = "Position Z:";
                unit1 = "m";
                unit2 = "m";
                unit3 = "m";
                break;
        }
    }

    /**
     * Get short label strings for compact UI.
     */
    inline void getShortLabels(Mode mode,
                               juce::String& label1, juce::String& label2, juce::String& label3)
    {
        switch (mode)
        {
            case Mode::Cylindrical:
                label1 = "r:";
                label2 = juce::String(juce::CharPointer_UTF8("\xce\xb8:"));  // theta
                label3 = "Z:";
                break;

            case Mode::Spherical:
                label1 = "r:";
                label2 = juce::String(juce::CharPointer_UTF8("\xce\xb8:"));  // theta
                label3 = juce::String(juce::CharPointer_UTF8("\xcf\x86:"));  // phi
                break;

            default: // Cartesian
                label1 = "X:";
                label2 = "Y:";
                label3 = "Z:";
                break;
        }
    }

    /**
     * Convert display values to Cartesian based on mode.
     * Takes values from UI editors (v1, v2, v3) and converts to Cartesian.
     */
    inline CartesianCoord displayToCartesian(Mode mode, float v1, float v2, float v3)
    {
        switch (mode)
        {
            case Mode::Cylindrical:
            {
                // v1=radius, v2=theta, v3=height
                CylindricalCoord cyl = { std::abs(v1), normalizeAngle(v2), v3 };
                return cylindricalToCartesian(cyl);
            }

            case Mode::Spherical:
            {
                // v1=radius, v2=theta, v3=phi
                SphericalCoord sph = { std::abs(v1), normalizeAngle(v2), clampElevation(v3) };
                return sphericalToCartesian(sph);
            }

            default: // Cartesian
                return { v1, v2, v3 };
        }
    }

    /**
     * Convert Cartesian to display values based on mode.
     * Returns values suitable for UI editors (v1, v2, v3).
     */
    inline void cartesianToDisplay(Mode mode, float x, float y, float z,
                                   float& v1, float& v2, float& v3)
    {
        switch (mode)
        {
            case Mode::Cylindrical:
            {
                auto cyl = cartesianToCylindrical({ x, y, z });
                v1 = cyl.r;
                v2 = cyl.theta;
                v3 = cyl.z;
                break;
            }

            case Mode::Spherical:
            {
                auto sph = cartesianToSpherical({ x, y, z });
                v1 = sph.r;
                v2 = sph.theta;
                v3 = sph.phi;
                break;
            }

            default: // Cartesian
                v1 = x;
                v2 = y;
                v3 = z;
                break;
        }
    }

} // namespace WFSCoordinates
