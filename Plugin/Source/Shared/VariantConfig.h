#pragma once

#include <array>
#include <juce_core/juce_core.h>

namespace wfs::plugin
{
    enum class AddressingScheme
    {
        NativeWFS,
        AdmOsc
    };

    struct PositionParamSpec
    {
        juce::String paramID;
        juce::String label;
        juce::String oscPath;
        float minValue;
        float maxValue;
        float defaultValue;
        juce::String unit;
        bool rampCapable;
    };

    struct VariantConfig
    {
        juce::String productName;
        juce::String bundleIdSuffix;
        juce::String coordinateTag;
        AddressingScheme addressing;
        std::array<PositionParamSpec, 3> positions;
        bool positionsWired;
    };
}
