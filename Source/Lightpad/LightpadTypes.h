/*
  ==============================================================================
    LightpadTypes.h
    Shared enums, structs, and zone encoding for ROLI Lightpad Block integration.
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <set>

//==============================================================================
// Pad layout info reported by LightpadManager after topology auto-detection
//==============================================================================
struct PadLayoutInfo
{
    int padIndex;       // 0-2, assigned by spatial sort (top-to-bottom, left-to-right)
    int layoutX;        // Block layout X position (from Block::getBlockAreaWithinLayout)
    int layoutY;        // Block layout Y position
    bool isMaster;      // true if this pad is the USB-connected master block
    bool isSplit;       // true if split into 4 quadrants
};

//==============================================================================
// Zone info decoded from a zone ID
//==============================================================================
struct LightpadZoneInfo
{
    int padIndex;       // 0-2
    int quadrant;       // 0-3 (0=full or top-left, 1=top-right, 2=bottom-left, 3=bottom-right)
    bool isSplit;       // true if pad is split into 4 quadrants
};

//==============================================================================
// Zone ID encoding/decoding
//   zoneId = padIndex * 4 + quadrant   (max 12 zones: 0-11)
//==============================================================================
inline int encodeZoneId (int padIndex, int quadrant)
{
    return padIndex * 4 + quadrant;
}

inline LightpadZoneInfo decodeZoneId (int zoneId)
{
    return { zoneId / 4, zoneId % 4, false };  // isSplit set by caller
}

// Returns the list of valid zone IDs for the current config
inline std::vector<int> getActiveZoneIds (int numPads, const bool split[3])
{
    std::vector<int> ids;

    for (int pad = 0; pad < numPads; ++pad)
    {
        if (split[pad])
        {
            for (int q = 0; q < 4; ++q)
                ids.push_back (encodeZoneId (pad, q));
        }
        else
        {
            ids.push_back (encodeZoneId (pad, 0));
        }
    }

    return ids;
}

// Display name for a zone (for combobox items)
inline juce::String getZoneDisplayName (int zoneId, const bool split[3])
{
    auto info = decodeZoneId (zoneId);
    juce::String padName = "Pad " + juce::String (info.padIndex + 1);

    if (split[info.padIndex])
    {
        static const char* romanNumerals[] = { "I", "II", "III", "IV" };
        return padName + juce::String::charToString (L'\u00B7') + " " + romanNumerals[info.quadrant];
    }

    return padName;
}

//==============================================================================
// Fixed zone colours (up to 12 zones)
//==============================================================================
namespace LightpadColours
{
    inline juce::Colour getZoneColour (int zoneId)
    {
        static const juce::Colour colours[] =
        {
            juce::Colour (0xFFE74C3C),     // 0: Red
            juce::Colour (0xFF3498DB),     // 1: Blue
            juce::Colour (0xFF2ECC71),     // 2: Green
            juce::Colour (0xFFF39C12),     // 3: Orange
            juce::Colour (0xFF9B59B6),     // 4: Purple
            juce::Colour (0xFF1ABC9C),     // 5: Teal
            juce::Colour (0xFFE91E63),     // 6: Pink
            juce::Colour (0xFF00BCD4),     // 7: Cyan
            juce::Colour (0xFFFF5722),     // 8: Deep Orange
            juce::Colour (0xFF8BC34A),     // 9: Light Green
            juce::Colour (0xFFFFEB3B),     // 10: Yellow
            juce::Colour (0xFF795548),     // 11: Brown
        };

        if (zoneId >= 0 && zoneId < 12)
            return colours[zoneId];

        return juce::Colour (0xFF444444);  // dim gray for unassigned
    }

    inline juce::Colour getUnassignedColour()
    {
        return juce::Colour (0xFF333333);
    }

    // Distinct pad colours for the mini-map widget (visually distinct from each other)
    inline juce::Colour getPadColour (int padIndex)
    {
        static const juce::Colour padColours[] =
        {
            juce::Colour (0xFFE74C3C),   // Pad 0: Red
            juce::Colour (0xFF9B59B6),   // Pad 1: Purple
            juce::Colour (0xFF1ABC9C),   // Pad 2: Teal
        };
        if (padIndex >= 0 && padIndex < 3)
            return padColours[padIndex];
        return getUnassignedColour();
    }
}

//==============================================================================
// Pixel font data for zone number display on 15x15 LED grid
//==============================================================================
namespace LightpadPixelFont
{
    // 5x7 pixel digits 1-3 (for full pad, centered on 15x15)
    static const uint8_t digit1[] =
    {
        0,1,0,0,0,
        1,1,0,0,0,
        0,1,0,0,0,
        0,1,0,0,0,
        0,1,0,0,0,
        0,1,0,0,0,
        1,1,1,0,0,
    };

    static const uint8_t digit2[] =
    {
        1,1,1,0,0,
        0,0,0,1,0,
        0,0,0,1,0,
        0,1,1,0,0,
        1,0,0,0,0,
        1,0,0,0,0,
        1,1,1,1,0,
    };

    static const uint8_t digit3[] =
    {
        1,1,1,0,0,
        0,0,0,1,0,
        0,0,0,1,0,
        0,1,1,0,0,
        0,0,0,1,0,
        0,0,0,1,0,
        1,1,1,0,0,
    };

    static const uint8_t* getDigit (int d)
    {
        switch (d)
        {
            case 1: return digit1;
            case 2: return digit2;
            case 3: return digit3;
            default: return nullptr;
        }
    }

    // 3x5 Roman numerals for split quadrants (centered in 7x7 quadrant)
    static const uint8_t romanI[] =
    {
        1,1,1,
        0,1,0,
        0,1,0,
        0,1,0,
        1,1,1,
    };

    static const uint8_t romanII[] =
    {
        1,0,1,
        1,0,1,
        1,0,1,
        1,0,1,
        1,0,1,
    };

    static const uint8_t romanIII[] =
    {
        1,0,1,0,1,
        1,0,1,0,1,
        1,0,1,0,1,
        1,0,1,0,1,
        1,0,1,0,1,
    };

    // IV: 5x5
    static const uint8_t romanIV[] =
    {
        1,0,1,0,1,
        1,0,1,0,1,
        1,0,1,0,1,
        1,0,0,1,0,
        1,0,0,1,0,
    };
}
