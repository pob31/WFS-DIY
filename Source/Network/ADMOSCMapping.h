#pragma once

#include <JuceHeader.h>
#include <cmath>
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "../Helpers/CoordinateConverter.h"

/**
 * ADM-OSC Mapping Logic
 *
 * Pure math for mapping between ADM-OSC normalized coordinates and
 * WFS-DIY internal Cartesian coordinates (meters).
 *
 * Cartesian mappings: piecewise linear with asymmetric stretch per axis/direction.
 * Polar mappings: azimuth offset/flip, elevation flip, distance min/max.
 *
 * ADM-OSC conventions:
 *   Cartesian: x=right, y=forward, z=up, range [-1, 1]
 *   Polar: azimuth 0=front(audience), positive=left, range [-180, 180]
 *          elevation [-90, 90], distance [0, 1]
 *
 * WFS-DIY conventions:
 *   Cartesian: x=right, y=upstage(back), z=up (meters)
 *   Azimuth: 0=upstage(+Y), 90=right(+X), via atan2(x, y)
 */
namespace ADMOSCMapping
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    //==========================================================================
    // Config Structs
    //==========================================================================

    struct AxisConfig
    {
        int   axisSwap       = 0;     // which ADM axis feeds this: 0=X, 1=Y, 2=Z
        bool  signFlip       = false;
        float centerOffset   = 0.0f;  // meters
        float breakpoint     = 0.5f;  // 0-1 normalized
        float posInnerWidth  = 5.0f;  // meters, positive direction
        float posOuterWidth  = 5.0f;
        float negInnerWidth  = 5.0f;  // meters, negative direction
        float negOuterWidth  = 5.0f;
    };

    struct CartesianMappingConfig
    {
        AxisConfig axes[3];  // [0]=internal X, [1]=internal Y, [2]=internal Z
    };

    struct PolarMappingConfig
    {
        float azimuthOffset  = 0.0f;   // degrees
        bool  azimuthFlip    = false;
        bool  elevationFlip  = false;
        // Distance mapping: 3-point piecewise linear (half-axis, 0→1)
        float distBreakpoint = 0.5f;   // normalized [0.01, 0.99]
        float distInner      = 5.0f;   // meters: center to breakpoint
        float distOuter      = 5.0f;   // meters: breakpoint to max
        float distCenter     = 0.0f;   // meters: center offset
    };

    //==========================================================================
    // Cartesian Mapping Math
    //==========================================================================

    /** Map a single normalized ADM-OSC axis value [-1, 1] to meters,
        using asymmetric piecewise linear stretch. */
    inline float mapCartesianAxis (float v, const AxisConfig& cfg)
    {
        if (cfg.signFlip) v = -v;

        float innerW, outerW;
        float sign;

        if (v >= 0.0f)
        {
            sign    = 1.0f;
            innerW  = cfg.posInnerWidth;
            outerW  = cfg.posOuterWidth;
        }
        else
        {
            sign    = -1.0f;
            innerW  = cfg.negInnerWidth;
            outerW  = cfg.negOuterWidth;
            v       = -v;  // work with absolute value
        }

        float result;
        float bp = juce::jlimit (0.01f, 0.99f, cfg.breakpoint);

        if (v <= bp)
            result = (v / bp) * innerW;
        else
            result = innerW + ((v - bp) / (1.0f - bp)) * outerW;

        return cfg.centerOffset + sign * result;
    }

    /** Inverse: convert meters back to normalized [-1, 1] for transmit. */
    inline float inverseCartesianAxis (float meters, const AxisConfig& cfg)
    {
        float local = meters - cfg.centerOffset;
        float sign  = (local >= 0.0f) ? 1.0f : -1.0f;
        float absLocal = std::abs (local);

        float innerW = (local >= 0.0f) ? cfg.posInnerWidth : cfg.negInnerWidth;
        float outerW = (local >= 0.0f) ? cfg.posOuterWidth : cfg.negOuterWidth;
        float bp = juce::jlimit (0.01f, 0.99f, cfg.breakpoint);

        float v;
        if (absLocal <= innerW)
            v = (absLocal / innerW) * bp;
        else
            v = bp + ((absLocal - innerW) / outerW) * (1.0f - bp);

        v = juce::jlimit (0.0f, 1.0f, v) * sign;

        if (cfg.signFlip) v = -v;
        return juce::jlimit (-1.0f, 1.0f, v);
    }

    /** Apply full Cartesian mapping: ADM-OSC xyz -> WFS Cartesian meters.
        Handles axis swap (which ADM axis feeds which internal axis). */
    inline WFSCoordinates::CartesianCoord applyCartesianMapping (
        float admX, float admY, float admZ,
        const CartesianMappingConfig& cfg)
    {
        float admValues[3] = { admX, admY, admZ };
        WFSCoordinates::CartesianCoord result;
        float* out[3] = { &result.x, &result.y, &result.z };

        for (int i = 0; i < 3; ++i)
        {
            int srcAxis = juce::jlimit (0, 2, cfg.axes[i].axisSwap);
            *out[i] = mapCartesianAxis (admValues[srcAxis], cfg.axes[i]);
        }

        return result;
    }

    /** Inverse full Cartesian mapping: WFS meters -> ADM-OSC xyz.
        Reconstructs ADM values by inverting each axis then un-swapping. */
    inline void inverseCartesianMapping (
        float x, float y, float z,
        const CartesianMappingConfig& cfg,
        float& admX, float& admY, float& admZ)
    {
        // First, invert each internal axis to get the "raw" value that was fed to it
        float rawPerAxis[3];
        float meters[3] = { x, y, z };

        for (int i = 0; i < 3; ++i)
            rawPerAxis[i] = inverseCartesianAxis (meters[i], cfg.axes[i]);

        // Then un-swap: rawPerAxis[i] was the ADM value at cfg.axes[i].axisSwap
        float admOut[3] = { 0.0f, 0.0f, 0.0f };
        for (int i = 0; i < 3; ++i)
        {
            int srcAxis = juce::jlimit (0, 2, cfg.axes[i].axisSwap);
            admOut[srcAxis] = rawPerAxis[i];
        }

        admX = admOut[0];
        admY = admOut[1];
        admZ = admOut[2];
    }

    //==========================================================================
    // Polar Mapping Math
    //==========================================================================

    /** Convert ADM-OSC aed to WFS Cartesian.
        ADM: az 0=front(audience), +left.  WFS: az 0=upstage(+Y), +right.
        Conversion: wfsAz = -(admAz) + 180, normalized to [-180, 180]. */
    inline WFSCoordinates::CartesianCoord applyPolarMapping (
        float admAz, float admEl, float admDist,
        const PolarMappingConfig& cfg)
    {
        // Apply azimuth flip and offset
        float az = cfg.azimuthFlip ? -admAz : admAz;
        az += cfg.azimuthOffset;

        // Apply elevation flip
        float el = cfg.elevationFlip ? -admEl : admEl;

        // Convert ADM azimuth convention to WFS convention
        // ADM 0 = front (audience) = WFS 180/-180
        // ADM +90 = left = WFS -90
        float wfsAz = WFSCoordinates::normalizeAngle (-az + 180.0f);

        // Map distance: 0..1 -> piecewise linear via breakpoint
        float bp = juce::jlimit (0.01f, 0.99f, cfg.distBreakpoint);
        float dist;
        if (admDist <= bp)
            dist = cfg.distCenter + (admDist / bp) * cfg.distInner;
        else
            dist = cfg.distCenter + cfg.distInner
                   + ((admDist - bp) / (1.0f - bp)) * cfg.distOuter;
        dist = juce::jmax (0.0f, dist);

        // Convert to Cartesian using WFS convention
        float elRad  = juce::degreesToRadians (el);
        float azRad  = juce::degreesToRadians (wfsAz);
        float rHoriz = dist * std::cos (elRad);

        return {
            rHoriz * std::sin (azRad),   // x = r * sin(theta)
            rHoriz * std::cos (azRad),   // y = r * cos(theta)
            dist   * std::sin (elRad)    // z = r * sin(phi)
        };
    }

    /** Inverse polar mapping: WFS Cartesian -> ADM-OSC aed. */
    inline void inversePolarMapping (
        float x, float y, float z,
        const PolarMappingConfig& cfg,
        float& admAz, float& admEl, float& admDist)
    {
        // Convert to WFS spherical
        auto sph = WFSCoordinates::cartesianToSpherical ({ x, y, z });

        // WFS azimuth to ADM azimuth: inverse of wfsAz = -az + 180
        // az = -(wfsAz - 180) = 180 - wfsAz
        float az = WFSCoordinates::normalizeAngle (180.0f - sph.theta);

        // Un-apply offset
        az -= cfg.azimuthOffset;

        // Un-apply flip
        if (cfg.azimuthFlip) az = -az;

        admAz = WFSCoordinates::normalizeAngle (az);

        // Elevation: un-apply flip
        admEl = cfg.elevationFlip ? -sph.phi : sph.phi;

        // Distance: inverse of piecewise linear mapping
        float bp = juce::jlimit (0.01f, 0.99f, cfg.distBreakpoint);
        float local = sph.r - cfg.distCenter;
        if (local <= cfg.distInner)
        {
            if (cfg.distInner > 0.001f)
                admDist = juce::jlimit (0.0f, 1.0f, (local / cfg.distInner) * bp);
            else
                admDist = 0.0f;
        }
        else
        {
            if (cfg.distOuter > 0.001f)
                admDist = juce::jlimit (0.0f, 1.0f,
                    bp + ((local - cfg.distInner) / cfg.distOuter) * (1.0f - bp));
            else
                admDist = 1.0f;
        }
    }

    //==========================================================================
    // ValueTree Loaders
    //==========================================================================

    /** Load a CartesianMappingConfig from a ADMCartMapping ValueTree node. */
    inline CartesianMappingConfig loadCartesianConfig (const juce::ValueTree& mappingNode)
    {
        CartesianMappingConfig cfg;

        for (int a = 0; a < 3; ++a)
        {
            // Find the axis child with admCartAxisId == a
            juce::ValueTree axisNode;
            for (int c = 0; c < mappingNode.getNumChildren(); ++c)
            {
                auto child = mappingNode.getChild (c);
                if (child.getType() == ADMCartAxis &&
                    static_cast<int> (child.getProperty (admCartAxisId)) == a)
                {
                    axisNode = child;
                    break;
                }
            }

            if (!axisNode.isValid()) continue;

            cfg.axes[a].axisSwap      = static_cast<int>   (axisNode.getProperty (admCartAxisSwap, a));
            cfg.axes[a].signFlip      = static_cast<int>   (axisNode.getProperty (admCartSignFlip, 0)) != 0;
            cfg.axes[a].centerOffset  = static_cast<float> (axisNode.getProperty (admCartCenterOffset, 0.0f));
            cfg.axes[a].breakpoint    = static_cast<float> (axisNode.getProperty (admCartBreakpoint, 0.5f));
            cfg.axes[a].posInnerWidth = static_cast<float> (axisNode.getProperty (admCartPosInnerWidth, admCartWidthDefault));
            cfg.axes[a].posOuterWidth = static_cast<float> (axisNode.getProperty (admCartPosOuterWidth, admCartWidthDefault));
            cfg.axes[a].negInnerWidth = static_cast<float> (axisNode.getProperty (admCartNegInnerWidth, admCartWidthDefault));
            cfg.axes[a].negOuterWidth = static_cast<float> (axisNode.getProperty (admCartNegOuterWidth, admCartWidthDefault));
        }

        return cfg;
    }

    /** Load a PolarMappingConfig from a ADMPolarMapping ValueTree node. */
    inline PolarMappingConfig loadPolarConfig (const juce::ValueTree& mappingNode)
    {
        PolarMappingConfig cfg;
        cfg.azimuthOffset = static_cast<float> (mappingNode.getProperty (admPolarAzimuthOffset, 0.0f));
        cfg.azimuthFlip   = static_cast<int>   (mappingNode.getProperty (admPolarAzimuthFlip, 0)) != 0;
        cfg.elevationFlip = static_cast<int>   (mappingNode.getProperty (admPolarElevationFlip, 0)) != 0;

        // New breakpoint-based distance parameters
        if (mappingNode.hasProperty (admPolarDistInner))
        {
            cfg.distBreakpoint = static_cast<float> (mappingNode.getProperty (admPolarDistBreakpoint, 0.5f));
            cfg.distInner      = static_cast<float> (mappingNode.getProperty (admPolarDistInner, 5.0f));
            cfg.distOuter      = static_cast<float> (mappingNode.getProperty (admPolarDistOuter, 5.0f));
            cfg.distCenter     = static_cast<float> (mappingNode.getProperty (admPolarDistCenter, 0.0f));
        }
        else if (mappingNode.hasProperty (admPolarDistMin) || mappingNode.hasProperty (admPolarDistMax))
        {
            // Migration from legacy distMin/distMax
            float oldMin = static_cast<float> (mappingNode.getProperty (admPolarDistMin, 0.0f));
            float oldMax = static_cast<float> (mappingNode.getProperty (admPolarDistMax, 10.0f));
            float range  = oldMax - oldMin;
            cfg.distCenter     = oldMin;
            cfg.distInner      = range * 0.5f;
            cfg.distOuter      = range * 0.5f;
            cfg.distBreakpoint = 0.5f;
        }

        return cfg;
    }

}  // namespace ADMOSCMapping
