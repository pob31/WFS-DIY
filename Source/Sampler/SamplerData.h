#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "../Parameters/VarCoercion.h"
#include <cmath>

namespace SamplerData
{

/** A sampler number on its way to the engine: absent, or not a finite number,
    reads as the default; anything else is held to its range. Every path to
    the engine - the tab, MCP, a project or snapshot load, an imported
    sampler file - goes through the two loaders below, and none of them was
    bounded: "+40" was +40 dB, "inf" an infinite gain, a NaN curve a NaN
    output (re-audit 2026-09-29, section 5). */
inline float readBounded (const juce::ValueTree& node, const juce::Identifier& id,
                          float defaultValue, float minValue, float maxValue)
{
    const auto v = node.getProperty (id);
    if (v.isVoid())
        return defaultValue;

    double d = 0.0;
    if (v.isString())
    {
        const auto parsed = WFSVar::parseFiniteNumber (v.toString());
        if (! parsed)
            return defaultValue;
        d = *parsed;
    }
    else
    {
        d = static_cast<double> (v);
    }

    if (! std::isfinite (d))
        return defaultValue;

    return juce::jlimit (minValue, maxValue, static_cast<float> (d));
}

/** 0 or 1, whatever was stored. */
inline int readFlag (const juce::ValueTree& node, const juce::Identifier& id, int defaultValue)
{
    return WFSVar::toInt (node.getProperty (id), defaultValue) != 0 ? 1 : 0;
}

/**
 * Represents a single sample cell in the 6x6 grid.
 * Holds the audio buffer and metadata for one sample slot.
 */
struct SampleCell
{
    juce::String name;                   // Display name (defaults to filename)
    juce::String relativeFilePath;       // Path relative to project samples/ folder
    float inTime  = 0.0f;               // ms — fade-in / start offset
    float outTime = 20.0f;              // ms — fade-out on release
    float offsetX = 0.0f;               // Position offset from set base (meters)
    float offsetY = 0.0f;
    float offsetZ = 0.0f;
    float attenuation = 0.0f;           // dB (0 = no change)

    // Audio data (loaded on demand, nullptr when empty)
    std::shared_ptr<juce::AudioBuffer<float>> audioBuffer;
    double sampleRate = 0.0;
    int numSamples = 0;

    bool hasAudio() const noexcept { return audioBuffer != nullptr && numSamples > 0; }
    bool isEmpty() const noexcept { return relativeFilePath.isEmpty(); }

    /** Load cell properties from a ValueTree node */
    void loadFromValueTree (const juce::ValueTree& node)
    {
        using namespace WFSParameterIDs;
        if (! node.isValid()) return;

        using namespace WFSParameterDefaults;
        name             = node.getProperty (samplerCellName, "").toString();
        relativeFilePath = node.getProperty (samplerCellFile, "").toString();
        inTime      = readBounded (node, samplerCellInTime, samplerCellInTimeDefault, samplerCellInTimeMin, samplerCellInTimeMax);
        outTime     = readBounded (node, samplerCellOutTime, samplerCellOutTimeDefault, samplerCellOutTimeMin, samplerCellOutTimeMax);
        offsetX     = readBounded (node, samplerCellOffsetX, samplerCellOffsetDefault, samplerCellOffsetMin, samplerCellOffsetMax);
        offsetY     = readBounded (node, samplerCellOffsetY, samplerCellOffsetDefault, samplerCellOffsetMin, samplerCellOffsetMax);
        offsetZ     = readBounded (node, samplerCellOffsetZ, samplerCellOffsetDefault, samplerCellOffsetMin, samplerCellOffsetMax);
        attenuation = readBounded (node, samplerCellAttenuation, samplerCellAttenuationDefault,
                                   samplerCellAttenuationMin, samplerCellAttenuationMax);
    }
};

/**
 * Pressure mapping configuration for one parameter (Level, Height, or HF Shelf).
 */
struct PressureMapping
{
    bool enabled = false;
    int direction = 0;                   // 0 = positive (more pressure → more effect), 1 = negative
    float curve = 0.5f;                  // 0.0-1.0 mapping curve (0.5 = linear)

    /** Apply the pressure curve to a normalized input [0,1] → output [0,1] */
    float apply (float normalizedPressure) const noexcept
    {
        if (! enabled) return 0.0f;

        // Power curve: curve < 0.5 = concave (sensitive at low pressure)
        //              curve > 0.5 = convex (sensitive at high pressure)
        //              curve = 0.5 = linear
        // A non-finite curve or pressure reads as linear / none: jlimit lets
        // NaN through, and a NaN here was a NaN output.
        const float c = std::isfinite (curve) ? curve : 0.5f;
        const float p = std::isfinite (normalizedPressure) ? juce::jlimit (0.0f, 1.0f, normalizedPressure) : 0.0f;
        float exponent = (c < 0.01f) ? 0.01f : (c > 0.99f ? 100.0f : c / (1.0f - c));
        float result = std::pow (p, exponent);

        return direction == 0 ? result : (1.0f - result);
    }
};

/**
 * A set is a user-defined group of cells that play in sequence or round-robin.
 */
struct SamplerSet
{
    juce::String name;
    int playMode = 0;                    // 0 = sequential, 1 = round-robin
    std::vector<int> cellIndices;        // Which cells (0-35) belong to this set

    // Base position (absolute stage coordinates)
    float posX = 0.0f;
    float posY = 0.0f;
    float posZ = 0.0f;
    float level = 0.0f;                  // dB

    // Pressure mappings (Level + XY Pos enabled by default)
    PressureMapping pressLevel { true, 0, 0.5f };
    PressureMapping pressZ;
    PressureMapping pressHF;
    bool pressXYEnabled = true;
    float pressXYScale = 0.05f;          // m per update (same as joystick)

    // Playback state (not serialized)
    int currentIndex = 0;                // Next cell to play
    std::vector<int> shuffledOrder;      // For round-robin mode
    juce::Random rng { static_cast<juce::int64> (juce::Time::currentTimeMillis()) };

    /** Parse comma-separated cell indices from a string */
    static std::vector<int> parseCellIndices (const juce::String& str)
    {
        std::vector<int> result;
        auto tokens = juce::StringArray::fromTokens (str, ",", "");
        for (auto& t : tokens)
        {
            int idx = t.trim().getIntValue();
            if (idx >= 0 && idx < WFSParameterDefaults::samplerGridCells)
                result.push_back (idx);
        }
        return result;
    }

    /** Serialize cell indices to a comma-separated string */
    static juce::String cellIndicesToString (const std::vector<int>& indices)
    {
        juce::StringArray tokens;
        for (int idx : indices)
            tokens.add (juce::String (idx));
        return tokens.joinIntoString (",");
    }

    /** Load set properties from a ValueTree node */
    void loadFromValueTree (const juce::ValueTree& node)
    {
        using namespace WFSParameterIDs;
        using namespace WFSParameterDefaults;
        if (! node.isValid()) return;

        name        = node.getProperty (samplerSetName, "").toString();
        playMode    = readFlag (node, samplerSetPlayMode, samplerSetPlayModeDefault);
        cellIndices = parseCellIndices (node.getProperty (samplerSetCells, "").toString());

        posX  = readBounded (node, samplerSetPosX, samplerSetPosDefault, samplerSetPosMin, samplerSetPosMax);
        posY  = readBounded (node, samplerSetPosY, samplerSetPosDefault, samplerSetPosMin, samplerSetPosMax);
        posZ  = readBounded (node, samplerSetPosZ, samplerSetPosDefault, samplerSetPosMin, samplerSetPosMax);
        level = readBounded (node, samplerSetLevel, samplerSetLevelDefault, samplerSetLevelMin, samplerSetLevelMax);

        const auto curveOf = [&node] (const juce::Identifier& property)
        {
            return readBounded (node, property, samplerSetPressCurveDefault, samplerSetPressCurveMin, samplerSetPressCurveMax);
        };

        pressLevel.enabled   = readFlag (node, samplerSetPressLevelEnabled, samplerSetPressLevelEnabledDefault) != 0;
        pressLevel.direction = readFlag (node, samplerSetPressLevelDir, samplerSetPressDirDefault);
        pressLevel.curve     = curveOf (samplerSetPressLevelCurve);

        pressZ.enabled   = readFlag (node, samplerSetPressZEnabled, samplerSetPressEnabledDefault) != 0;
        pressZ.direction = readFlag (node, samplerSetPressZDir, samplerSetPressDirDefault);
        pressZ.curve     = curveOf (samplerSetPressZCurve);

        pressHF.enabled   = readFlag (node, samplerSetPressHFEnabled, samplerSetPressEnabledDefault) != 0;
        pressHF.direction = readFlag (node, samplerSetPressHFDir, samplerSetPressDirDefault);
        pressHF.curve     = curveOf (samplerSetPressHFCurve);

        pressXYEnabled = readFlag (node, samplerSetPressXYEnabled, samplerSetPressXYEnabledDefault) != 0;
        pressXYScale   = readBounded (node, samplerSetPressXYScale, samplerSetPressXYScaleDefault,
                                      samplerSetPressXYScaleMin, samplerSetPressXYScaleMax);

        // Reset playback state
        currentIndex = 0;
        shuffledOrder.clear();
    }

    /** Get the next cell index to play based on play mode */
    int getNextCellIndex()
    {
        if (cellIndices.empty()) return -1;

        if (playMode == 0)
        {
            // Sequential
            int idx = cellIndices[currentIndex % cellIndices.size()];
            currentIndex = (currentIndex + 1) % static_cast<int> (cellIndices.size());
            return idx;
        }
        else
        {
            // Round-robin (play all before reshuffling)
            if (shuffledOrder.empty())
            {
                shuffledOrder = cellIndices;
                for (int i = static_cast<int> (shuffledOrder.size()) - 1; i > 0; --i)
                    std::swap (shuffledOrder[static_cast<size_t> (i)],
                               shuffledOrder[static_cast<size_t> (rng.nextInt (i + 1))]);
                currentIndex = 0;
            }

            int idx = shuffledOrder[static_cast<size_t> (currentIndex)];
            currentIndex++;

            if (currentIndex >= static_cast<int> (shuffledOrder.size()))
                shuffledOrder.clear();  // Will reshuffle on next call

            return idx;
        }
    }
};

} // namespace SamplerData
