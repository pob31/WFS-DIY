#pragma once

/**
 * WFSConstraints — Shared position/offset constraint utility.
 *
 * Provides stage-aware constraint logic used by:
 *   - InputsTab (number box commit)
 *   - MapTab (mouse/touch drag)
 *   - Stream Deck+ dials (MapTabPages)
 *
 * All functions operate on Cartesian (x, y, z) values and read constraint
 * flags + stage config from WFSValueTreeState.
 */

#include "WFSValueTreeState.h"
#include "WFSParameterIDs.h"
#include "WFSParameterDefaults.h"
#include <JuceHeader.h>

namespace WFSConstraints
{

//==============================================================================
// Stage Bounds
//==============================================================================

struct StageBounds
{
    float minX, maxX;
    float minY, maxY;
    float minZ, maxZ;
};

/** Compute origin-relative stage bounds from config parameters.
    Accounts for stage shape (box/cylinder/dome), dimensions, and origin offsets.
*/
inline StageBounds getStageBounds (WFSValueTreeState& state)
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    auto config = state.getConfigState();

    // Helper to read a config property with a default fallback
    auto getConfig = [&config] (const juce::Identifier& id, float defaultVal) -> float
    {
        for (int i = 0; i < config.getNumChildren(); ++i)
        {
            auto child = config.getChild (i);
            if (child.hasProperty (id))
                return static_cast<float> (child.getProperty (id));
        }
        return defaultVal;
    };

    int shape = static_cast<int> (getConfig (stageShape, static_cast<float> (stageShapeDefault)));

    float halfSizeW = (shape == 0)
        ? getConfig (stageWidth, stageWidthDefault) / 2.0f
        : getConfig (stageDiameter, stageDiameterDefault) / 2.0f;

    float halfSizeD = (shape == 0)
        ? getConfig (stageDepth, stageDepthDefault) / 2.0f
        : getConfig (stageDiameter, stageDiameterDefault) / 2.0f;

    float originW = getConfig (originWidth, originWidthDefault);
    float originD = getConfig (originDepth, originDepthDefault);
    float originH = getConfig (originHeight, originHeightDefault);
    float stageH  = getConfig (stageHeight, stageHeightDefault);

    return {
        -halfSizeW - originW,  halfSizeW - originW,   // X
        -halfSizeD - originD,  halfSizeD - originD,    // Y
        -originH,              stageH - originH         // Z
    };
}

//==============================================================================
// Constrain Position (Cartesian)
//==============================================================================

/** Constrain Cartesian position values for a given input channel.
    Reads coordinate mode, per-axis constraint flags, and distance constraint
    parameters from the input's ValueTree state.

    @param state         The application's value tree state
    @param channelIndex  0-based input channel index
    @param x, y, z       Position values to constrain (modified in place)
*/
inline void constrainPosition (WFSValueTreeState& state, int channelIndex,
                                float& x, float& y, float& z)
{
    using namespace WFSParameterIDs;

    int coordMode   = static_cast<int> (state.getInputParameter (channelIndex, inputCoordinateMode));
    int constrainX  = static_cast<int> (state.getInputParameter (channelIndex, inputConstraintX));
    int constrainY  = static_cast<int> (state.getInputParameter (channelIndex, inputConstraintY));
    int constrainZ  = static_cast<int> (state.getInputParameter (channelIndex, inputConstraintZ));
    int constrainD  = static_cast<int> (state.getInputParameter (channelIndex, inputConstraintDistance));

    auto bounds = getStageBounds (state);

    bool useDistance = (coordMode == 1 || coordMode == 2) && (constrainD != 0);

    if (useDistance)
    {
        float minDist = static_cast<float> (state.getInputParameter (channelIndex, inputConstraintDistanceMin));
        float maxDist = static_cast<float> (state.getInputParameter (channelIndex, inputConstraintDistanceMax));

        float currentDist = (coordMode == 1)
            ? std::sqrt (x * x + y * y)                      // Cylindrical: XY plane
            : std::sqrt (x * x + y * y + z * z);             // Spherical: full 3D

        if (currentDist < 0.0001f)
            currentDist = 0.0001f;

        float targetDist = juce::jlimit (minDist, maxDist, currentDist);

        if (! juce::approximatelyEqual (currentDist, targetDist))
        {
            float scale = targetDist / currentDist;
            x *= scale;
            y *= scale;
            if (coordMode == 2)  // Spherical: also scale Z
                z *= scale;
        }

        // Z rectangular constraint in cylindrical mode (distance only constrains XY)
        if (coordMode == 1 && constrainZ != 0)
            z = juce::jlimit (bounds.minZ, bounds.maxZ, z);
    }
    else
    {
        // Rectangular (Cartesian) constraints
        if (constrainX != 0) x = juce::jlimit (bounds.minX, bounds.maxX, x);
        if (constrainY != 0) y = juce::jlimit (bounds.minY, bounds.maxY, y);
        if (constrainZ != 0) z = juce::jlimit (bounds.minZ, bounds.maxZ, z);
    }
}

//==============================================================================
// Constrain Offset
//==============================================================================

/** Constrain offset values so that (position + offset) stays within bounds.
    Reads current position from the input's parameters internally.

    @param state         The application's value tree state
    @param channelIndex  0-based input channel index
    @param offX, offY, offZ  Offset values to constrain (modified in place)
*/
inline void constrainOffset (WFSValueTreeState& state, int channelIndex,
                              float& offX, float& offY, float& offZ)
{
    using namespace WFSParameterIDs;

    float posX = static_cast<float> (state.getInputParameter (channelIndex, inputPositionX));
    float posY = static_cast<float> (state.getInputParameter (channelIndex, inputPositionY));
    float posZ = static_cast<float> (state.getInputParameter (channelIndex, inputPositionZ));

    // Compute total position
    float totalX = posX + offX;
    float totalY = posY + offY;
    float totalZ = posZ + offZ;

    // Apply the same constraints to the total position
    constrainPosition (state, channelIndex, totalX, totalY, totalZ);

    // Recompute offset from constrained total
    offX = totalX - posX;
    offY = totalY - posY;
    offZ = totalZ - posZ;
}

} // namespace WFSConstraints
