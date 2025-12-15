#pragma once

#include <JuceHeader.h>

/**
 * Shared color utilities for consistent coloring across the application.
 * Used by MapTab for markers and by ChannelSelector for channel buttons.
 */
namespace WfsColorUtilities
{
    /**
     * Get marker color for inputs or clusters/arrays.
     * Matches the Android WFS Control app color scheme.
     *
     * @param id The marker ID (1-based for inputs, 1-10 for clusters/arrays)
     * @param isClusterMarker true for cluster/array colors, false for input colors
     * @return HSL-based color for the marker
     */
    inline juce::Colour getMarkerColor(int id, bool isClusterMarker = false)
    {
        // HSL-based colors matching Android app
        // For inputs: hue = (id * 360 / 32) % 360, saturation = 0.9, lightness = 0.6
        // For clusters/arrays: hue = (id * 360 / 10) % 360, saturation = 0.7, lightness = 0.7
        int totalMarkers = isClusterMarker ? 10 : 32;
        float hue = std::fmod((id * 360.0f / totalMarkers), 360.0f) / 360.0f;
        float saturation = isClusterMarker ? 0.7f : 0.9f;
        float lightness = isClusterMarker ? 0.7f : 0.6f;
        return juce::Colour::fromHSL(hue, saturation, lightness, 1.0f);
    }

    /**
     * Get color for an array (convenience wrapper).
     *
     * @param arrayNumber Array number (1-10)
     * @return HSL-based color for the array
     */
    inline juce::Colour getArrayColor(int arrayNumber)
    {
        return getMarkerColor(arrayNumber, true);
    }

    /**
     * Get color for an input marker (convenience wrapper).
     *
     * @param inputId Input ID (1-32)
     * @return HSL-based color for the input
     */
    inline juce::Colour getInputColor(int inputId)
    {
        return getMarkerColor(inputId, false);
    }
}
