#pragma once

#include <JuceHeader.h>
#include "../Shared/PluginAdmMapping.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "../Helpers/CoordinateConverter.h"

/**
 * App-side ADM-OSC mapping.
 *
 * The pure math (axis swap/flip/breakpoint, polar conversion, inverses) lives
 * in Source/Shared/PluginAdmMapping.h so the plugin set can include it without
 * dragging in app-only headers. This header keeps the existing
 * `ADMOSCMapping::...` API the rest of the app calls, and adds the ValueTree
 * loaders that read mapping configs out of the app's parameter tree.
 */
namespace ADMOSCMapping
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    using AxisConfig             = PluginAdmMapping::AxisConfig;
    using CartesianMappingConfig = PluginAdmMapping::CartesianMappingConfig;
    using PolarMappingConfig     = PluginAdmMapping::PolarMappingConfig;

    using PluginAdmMapping::mapCartesianAxis;
    using PluginAdmMapping::inverseCartesianAxis;
    using PluginAdmMapping::inverseCartesianMapping;
    using PluginAdmMapping::inversePolarMapping;

    /** App-flavoured wrappers that return WFSCoordinates::CartesianCoord. */
    inline WFSCoordinates::CartesianCoord applyCartesianMapping (
        float admX, float admY, float admZ,
        const CartesianMappingConfig& cfg)
    {
        const auto v = PluginAdmMapping::applyCartesianMapping (admX, admY, admZ, cfg);
        return { v.x, v.y, v.z };
    }

    inline WFSCoordinates::CartesianCoord applyPolarMapping (
        float admAz, float admEl, float admDist,
        const PolarMappingConfig& cfg)
    {
        const auto v = PluginAdmMapping::applyPolarMapping (admAz, admEl, admDist, cfg);
        return { v.x, v.y, v.z };
    }

    //==========================================================================
    // ValueTree Loaders (app-side only)
    //==========================================================================

    inline CartesianMappingConfig loadCartesianConfig (const juce::ValueTree& mappingNode)
    {
        CartesianMappingConfig cfg;

        for (int a = 0; a < 3; ++a)
        {
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

    inline PolarMappingConfig loadPolarConfig (const juce::ValueTree& mappingNode)
    {
        PolarMappingConfig cfg;
        cfg.azimuthOffset = static_cast<float> (mappingNode.getProperty (admPolarAzimuthOffset, 0.0f));
        cfg.azimuthFlip   = static_cast<int>   (mappingNode.getProperty (admPolarAzimuthFlip, 0)) != 0;
        cfg.elevationFlip = static_cast<int>   (mappingNode.getProperty (admPolarElevationFlip, 0)) != 0;

        if (mappingNode.hasProperty (admPolarDistInner))
        {
            cfg.distBreakpoint = static_cast<float> (mappingNode.getProperty (admPolarDistBreakpoint, 0.5f));
            cfg.distInner      = static_cast<float> (mappingNode.getProperty (admPolarDistInner, 5.0f));
            cfg.distOuter      = static_cast<float> (mappingNode.getProperty (admPolarDistOuter, 5.0f));
            cfg.distCenter     = static_cast<float> (mappingNode.getProperty (admPolarDistCenter, 0.0f));
        }
        else if (mappingNode.hasProperty (admPolarDistMin) || mappingNode.hasProperty (admPolarDistMax))
        {
            const float oldMin = static_cast<float> (mappingNode.getProperty (admPolarDistMin, 0.0f));
            const float oldMax = static_cast<float> (mappingNode.getProperty (admPolarDistMax, 10.0f));
            const float range  = oldMax - oldMin;
            cfg.distCenter     = oldMin;
            cfg.distInner      = range * 0.5f;
            cfg.distOuter      = range * 0.5f;
            cfg.distBreakpoint = 0.5f;
        }

        return cfg;
    }
}
