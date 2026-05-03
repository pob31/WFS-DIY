#pragma once

#include <juce_core/juce_core.h>
#include <cmath>

/**
 * Pure-math ADM-OSC <-> Cartesian mapping.
 *
 * This header is shared between the WFS-DIY app and the WFS-DIY plugin set.
 * It MUST stay free of app-only deps (no WFSParameterIDs, no ValueTree).
 * It depends only on juce_core (for jlimit) and <cmath>.
 *
 * The app-side header Source/Network/ADMOSCMapping.h re-exports these symbols
 * and adds the ValueTree loaders that read CartesianMappingConfig /
 * PolarMappingConfig out of the app's parameter tree.
 *
 * Conventions:
 *   ADM-OSC Cartesian: x=right, y=forward, z=up, range [-1, 1]
 *   ADM-OSC Polar:     azimuth 0=front (audience), positive=left, range [-180, 180]
 *                      elevation [-90, 90], distance [0, 1]
 *   WFS-DIY Cartesian: x=right, y=upstage (back), z=up (meters)
 *   WFS-DIY azimuth:   0=upstage (+Y), 90=right (+X)
 */
namespace PluginAdmMapping
{
    struct Vec3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct AxisConfig
    {
        int   axisSwap       = 0;
        bool  signFlip       = false;
        float centerOffset   = 0.0f;
        float breakpoint     = 0.5f;
        float posInnerWidth  = 5.0f;
        float posOuterWidth  = 5.0f;
        float negInnerWidth  = 5.0f;
        float negOuterWidth  = 5.0f;
    };

    struct CartesianMappingConfig
    {
        AxisConfig axes[3];
    };

    struct PolarMappingConfig
    {
        float azimuthOffset  = 0.0f;
        bool  azimuthFlip    = false;
        bool  elevationFlip  = false;
        float distBreakpoint = 0.5f;
        float distInner      = 5.0f;
        float distOuter      = 5.0f;
        float distCenter     = 0.0f;
    };

    inline float normalizeAngleDeg (float degrees)
    {
        while (degrees > 180.0f)   degrees -= 360.0f;
        while (degrees <= -180.0f) degrees += 360.0f;
        return degrees;
    }

    inline float mapCartesianAxis (float v, const AxisConfig& cfg)
    {
        if (cfg.signFlip) v = -v;

        float innerW, outerW, sign;
        if (v >= 0.0f)
        {
            sign   = 1.0f;
            innerW = cfg.posInnerWidth;
            outerW = cfg.posOuterWidth;
        }
        else
        {
            sign   = -1.0f;
            innerW = cfg.negInnerWidth;
            outerW = cfg.negOuterWidth;
            v      = -v;
        }

        const float bp = juce::jlimit (0.01f, 0.99f, cfg.breakpoint);
        float result;
        if (v <= bp) result = (v / bp) * innerW;
        else         result = innerW + ((v - bp) / (1.0f - bp)) * outerW;

        return cfg.centerOffset + sign * result;
    }

    inline float inverseCartesianAxis (float meters, const AxisConfig& cfg)
    {
        const float local    = meters - cfg.centerOffset;
        const float sign     = (local >= 0.0f) ? 1.0f : -1.0f;
        const float absLocal = std::abs (local);
        const float innerW   = (local >= 0.0f) ? cfg.posInnerWidth : cfg.negInnerWidth;
        const float outerW   = (local >= 0.0f) ? cfg.posOuterWidth : cfg.negOuterWidth;
        const float bp       = juce::jlimit (0.01f, 0.99f, cfg.breakpoint);

        float v;
        if (absLocal <= innerW) v = (absLocal / innerW) * bp;
        else                    v = bp + ((absLocal - innerW) / outerW) * (1.0f - bp);

        v = juce::jlimit (0.0f, 1.0f, v) * sign;
        if (cfg.signFlip) v = -v;
        return juce::jlimit (-1.0f, 1.0f, v);
    }

    inline Vec3 applyCartesianMapping (float admX, float admY, float admZ,
                                       const CartesianMappingConfig& cfg)
    {
        const float in[3]  = { admX, admY, admZ };
        Vec3 out;
        float* outPtr[3] = { &out.x, &out.y, &out.z };

        for (int i = 0; i < 3; ++i)
        {
            const int srcAxis = juce::jlimit (0, 2, cfg.axes[i].axisSwap);
            *outPtr[i] = mapCartesianAxis (in[srcAxis], cfg.axes[i]);
        }
        return out;
    }

    inline void inverseCartesianMapping (float x, float y, float z,
                                         const CartesianMappingConfig& cfg,
                                         float& admX, float& admY, float& admZ)
    {
        float rawPerAxis[3];
        const float meters[3] = { x, y, z };

        for (int i = 0; i < 3; ++i)
            rawPerAxis[i] = inverseCartesianAxis (meters[i], cfg.axes[i]);

        float admOut[3] = { 0.0f, 0.0f, 0.0f };
        for (int i = 0; i < 3; ++i)
        {
            const int srcAxis = juce::jlimit (0, 2, cfg.axes[i].axisSwap);
            admOut[srcAxis] = rawPerAxis[i];
        }

        admX = admOut[0];
        admY = admOut[1];
        admZ = admOut[2];
    }

    inline Vec3 applyPolarMapping (float admAz, float admEl, float admDist,
                                   const PolarMappingConfig& cfg)
    {
        float az = cfg.azimuthFlip ? -admAz : admAz;
        az += cfg.azimuthOffset;
        float el = cfg.elevationFlip ? -admEl : admEl;

        const float wfsAz = normalizeAngleDeg (-az + 180.0f);

        const float bp = juce::jlimit (0.01f, 0.99f, cfg.distBreakpoint);
        float dist;
        if (admDist <= bp) dist = cfg.distCenter + (admDist / bp) * cfg.distInner;
        else               dist = cfg.distCenter + cfg.distInner
                                  + ((admDist - bp) / (1.0f - bp)) * cfg.distOuter;
        dist = juce::jmax (0.0f, dist);

        const float elRad  = juce::degreesToRadians (el);
        const float azRad  = juce::degreesToRadians (wfsAz);
        const float rHoriz = dist * std::cos (elRad);

        return { rHoriz * std::sin (azRad),
                 rHoriz * std::cos (azRad),
                 dist   * std::sin (elRad) };
    }

    inline void inversePolarMapping (float x, float y, float z,
                                     const PolarMappingConfig& cfg,
                                     float& admAz, float& admEl, float& admDist)
    {
        const float r = std::sqrt (x * x + y * y + z * z);

        float thetaDeg = 0.0f;
        float phiDeg   = 0.0f;
        if (r > 0.0001f)
        {
            phiDeg = juce::radiansToDegrees (std::asin (z / r));
            const float rHoriz = std::sqrt (x * x + y * y);
            if (rHoriz > 0.0001f)
                thetaDeg = juce::radiansToDegrees (std::atan2 (x, y));
        }
        thetaDeg = normalizeAngleDeg (thetaDeg);
        phiDeg   = juce::jlimit (-90.0f, 90.0f, phiDeg);

        float az = normalizeAngleDeg (180.0f - thetaDeg);
        az -= cfg.azimuthOffset;
        if (cfg.azimuthFlip) az = -az;

        admAz = normalizeAngleDeg (az);
        admEl = cfg.elevationFlip ? -phiDeg : phiDeg;

        const float bp    = juce::jlimit (0.01f, 0.99f, cfg.distBreakpoint);
        const float local = r - cfg.distCenter;
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
}
