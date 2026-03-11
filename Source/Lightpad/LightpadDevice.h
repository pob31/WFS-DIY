/*
  ==============================================================================
    LightpadDevice.h
    Wraps a single ROLI Lightpad Block.
    Handles touch events (virtual joystick) and LED rendering (zone colours,
    touch disc, zone numbers).
  ==============================================================================
*/

#pragma once

#include <roli_blocks_basics/roli_blocks_basics.h>
#include "LightpadTypes.h"

class LightpadDevice : public roli::TouchSurface::Listener,
                        public roli::Block::ProgramLoadedListener
{
public:
    //==============================================================================
    LightpadDevice (roli::Block::Ptr blockPtr, int assignedPadIndex)
        : block (blockPtr), padIndex (assignedPadIndex)
    {
        jassert (block != nullptr);

        if (auto* ts = block->getTouchSurface())
            ts->addListener (this);

        block->addProgramLoadedListener (this);

        surfaceWidth  = static_cast<float> (block->getWidth());
        surfaceHeight = static_cast<float> (block->getHeight());

        loadBitmapProgram();
    }

    ~LightpadDevice() override
    {
        if (auto* ts = block->getTouchSurface())
            ts->removeListener (this);

        block->removeProgramLoadedListener (this);
    }

    //==============================================================================
    // Configuration
    //==============================================================================
    void setPadIndex (int idx)              { padIndex = idx; }
    int  getPadIndex() const                { return padIndex; }

    void setSplit (bool shouldSplit)
    {
        if (isSplit != shouldSplit)
        {
            isSplit = shouldSplit;
            renderCurrentState();
        }
    }

    bool getIsSplit() const                 { return isSplit; }

    roli::Block::Ptr getBlock() const       { return block; }
    roli::Block::UID getBlockUID() const    { return block->uid; }

    //==============================================================================
    // Zone colours (set by manager based on assignments)
    //==============================================================================
    void setZoneColour (int quadrant, juce::Colour colour)
    {
        if (quadrant >= 0 && quadrant < 4)
        {
            zoneColours[quadrant] = colour;
            renderCurrentState();
        }
    }

    void setAllZoneColours (const juce::Colour colours[4])
    {
        for (int i = 0; i < 4; ++i)
            zoneColours[i] = colours[i];

        renderCurrentState();
    }

    //==============================================================================
    // Zone number display mode (activated when combobox opens)
    //==============================================================================
    void setShowZoneNumbers (bool show)
    {
        if (showingNumbers != show)
        {
            showingNumbers = show;
            renderCurrentState();
        }
    }

    //==============================================================================
    // Touch callback — set by LightpadManager
    //==============================================================================
    std::function<void (int zoneId, float dx, float dy, float pressure,
                        bool isStart, bool isEnd)> onZoneTouch;

    //==============================================================================
    // TouchSurface::Listener
    //==============================================================================
    void touchChanged (roli::TouchSurface& surface, const roli::TouchSurface::Touch& touch) override
    {
        juce::ignoreUnused (surface);

        int touchIdx = touch.index;
        if (touchIdx < 0 || touchIdx >= maxTouches) return;

        // Determine which zone this touch is in (from initial position)
        int quadrant = getQuadrantForPosition (touch.startX, touch.startY);
        int zoneId   = encodeZoneId (padIndex, quadrant);

        // Compute virtual joystick deflection from strike point
        float dx = (touch.x - touch.startX) / surfaceWidth;   // normalised 0-1 range
        float dy = (touch.y - touch.startY) / surfaceHeight;

        if (touch.isTouchStart)
        {
            activeTouches[touchIdx] = { true, quadrant, touch.startX, touch.startY };
        }

        bool isEnd = touch.isTouchEnd;

        if (onZoneTouch)
            onZoneTouch (zoneId, dx, dy, touch.z, touch.isTouchStart, isEnd);

        // LED feedback: show white disc at touch position
        if (! showingNumbers)
        {
            if (isEnd)
            {
                activeTouches[touchIdx].active = false;
                renderCurrentState();
            }
            else
            {
                renderZonesWithTouchDisc (touch.x, touch.y, touch.z);
            }
        }

        if (isEnd)
            activeTouches[touchIdx].active = false;
    }

    //==============================================================================
    // Block::ProgramLoadedListener
    //==============================================================================
    void handleProgramLoaded (roli::Block&) override
    {
        programReady = true;
        renderCurrentState();
    }

private:
    //==============================================================================
    roli::Block::Ptr block;
    int padIndex = 0;
    bool isSplit = false;
    bool showingNumbers = false;
    bool programReady = false;

    float surfaceWidth  = 2.0f;
    float surfaceHeight = 2.0f;

    juce::Colour zoneColours[4] =
    {
        LightpadColours::getUnassignedColour(),
        LightpadColours::getUnassignedColour(),
        LightpadColours::getUnassignedColour(),
        LightpadColours::getUnassignedColour()
    };

    static constexpr int maxTouches = 16;

    struct ActiveTouch
    {
        bool  active   = false;
        int   quadrant = 0;
        float anchorX  = 0.0f;
        float anchorY  = 0.0f;
    };

    ActiveTouch activeTouches[maxTouches] = {};

    roli::BitmapLEDProgram* bitmapProgram = nullptr;

    //==============================================================================
    void loadBitmapProgram()
    {
        auto prog = std::make_unique<roli::BitmapLEDProgram> (*block);
        bitmapProgram = prog.get();
        block->setProgram (std::move (prog));
    }

    //==============================================================================
    int getQuadrantForPosition (float x, float y) const
    {
        if (! isSplit)
            return 0;

        float midX = surfaceWidth  * 0.5f;
        float midY = surfaceHeight * 0.5f;

        bool isRight  = (x >= midX);
        bool isBottom = (y >= midY);

        // 0=top-left, 1=top-right, 2=bottom-left, 3=bottom-right
        return (isBottom ? 2 : 0) + (isRight ? 1 : 0);
    }

    //==============================================================================
    // LED Rendering
    //==============================================================================
    static constexpr int gridSize = 15;

    void renderCurrentState()
    {
        if (bitmapProgram == nullptr || ! programReady) return;

        if (showingNumbers)
            renderZoneNumbers();
        else
            renderZoneColours();
    }

    void renderZoneColours()
    {
        if (bitmapProgram == nullptr) return;

        for (int y = 0; y < gridSize; ++y)
        {
            for (int x = 0; x < gridSize; ++x)
            {
                juce::Colour c;

                if (isSplit)
                {
                    // Cross separator at pixel 7 (center)
                    if (x == 7 || y == 7)
                    {
                        c = juce::Colours::black;
                    }
                    else
                    {
                        int quad = (y > 7 ? 2 : 0) + (x > 7 ? 1 : 0);
                        c = zoneColours[quad];
                    }
                }
                else
                {
                    c = zoneColours[0];
                }

                bitmapProgram->setLED (static_cast<juce::uint32> (x),
                                       static_cast<juce::uint32> (y),
                                       roli::LEDColour (c.getARGB()));
            }
        }
    }

    void renderZonesWithTouchDisc (float touchX, float touchY, float pressure)
    {
        if (bitmapProgram == nullptr) return;

        // First render zone colours
        renderZoneColours();

        // Map touch coordinates to LED grid coordinates
        int ledX = juce::jlimit (0, gridSize - 1,
                                  static_cast<int> ((touchX / surfaceWidth)  * gridSize));
        int ledY = juce::jlimit (0, gridSize - 1,
                                  static_cast<int> ((touchY / surfaceHeight) * gridSize));

        // Disc radius based on pressure (square-root curve for progressive growth)
        // Half size normally (1-3 px), quarter when split (0.5-1.5 px)
        float baseRadius = 1.0f + std::sqrt (pressure) * 2.0f;
        float radius = isSplit ? baseRadius * 0.5f : baseRadius;

        // Draw white disc
        for (int y = 0; y < gridSize; ++y)
        {
            for (int x = 0; x < gridSize; ++x)
            {
                float dist = std::sqrt (static_cast<float> ((x - ledX) * (x - ledX)
                                                           + (y - ledY) * (y - ledY)));
                if (dist <= radius)
                {
                    float brightness = 1.0f - (dist / radius) * 0.3f;
                    auto white = juce::Colour::fromFloatRGBA (brightness, brightness, brightness, 1.0f);
                    bitmapProgram->setLED (static_cast<juce::uint32> (x),
                                           static_cast<juce::uint32> (y),
                                           roli::LEDColour (white.getARGB()));
                }
            }
        }
    }

    void renderZoneNumbers()
    {
        if (bitmapProgram == nullptr) return;

        // Clear to zone colours first
        renderZoneColours();

        if (isSplit)
        {
            // Draw Roman numerals in each quadrant (7x7 pixel areas)
            renderRomanNumeral (0, 0, 0);   // top-left = I
            renderRomanNumeral (1, 8, 0);   // top-right = II
            renderRomanNumeral (2, 0, 8);   // bottom-left = III
            renderRomanNumeral (3, 8, 8);   // bottom-right = IV
        }
        else
        {
            // Draw large digit centered on 15x15
            renderLargeDigit (padIndex + 1);
        }
    }

    void renderRomanNumeral (int quadrant, int offsetX, int offsetY)
    {
        const uint8_t* glyph = nullptr;
        int glyphW = 3, glyphH = 5;

        switch (quadrant)
        {
            case 0: glyph = LightpadPixelFont::romanI;   glyphW = 3; break;
            case 1: glyph = LightpadPixelFont::romanII;  glyphW = 3; break;
            case 2: glyph = LightpadPixelFont::romanIII; glyphW = 5; break;
            case 3: glyph = LightpadPixelFont::romanIV;  glyphW = 5; break;
            default: return;
        }

        // Center glyph in the 7x7 quadrant area
        int startX = offsetX + (7 - glyphW) / 2;
        int startY = offsetY + (7 - glyphH) / 2;

        for (int gy = 0; gy < glyphH; ++gy)
        {
            for (int gx = 0; gx < glyphW; ++gx)
            {
                if (glyph[gy * glyphW + gx])
                {
                    int px = startX + gx;
                    int py = startY + gy;

                    if (px >= 0 && px < gridSize && py >= 0 && py < gridSize)
                    {
                        bitmapProgram->setLED (static_cast<juce::uint32> (px),
                                               static_cast<juce::uint32> (py),
                                               roli::LEDColour (juce::Colours::white.getARGB()));
                    }
                }
            }
        }
    }

    void renderLargeDigit (int digit)
    {
        const uint8_t* glyph = LightpadPixelFont::getDigit (digit);
        if (glyph == nullptr) return;

        int glyphW = 5, glyphH = 7;

        // Scale 2x and center on 15x15
        int scaledW = glyphW * 2;
        int scaledH = glyphH * 2;
        int startX  = (gridSize - scaledW) / 2;
        int startY  = (gridSize - scaledH) / 2;

        for (int gy = 0; gy < glyphH; ++gy)
        {
            for (int gx = 0; gx < glyphW; ++gx)
            {
                if (glyph[gy * glyphW + gx])
                {
                    // 2x scale
                    for (int sy = 0; sy < 2; ++sy)
                    {
                        for (int sx = 0; sx < 2; ++sx)
                        {
                            int px = startX + gx * 2 + sx;
                            int py = startY + gy * 2 + sy;

                            if (px >= 0 && px < gridSize && py >= 0 && py < gridSize)
                            {
                                bitmapProgram->setLED (static_cast<juce::uint32> (px),
                                                       static_cast<juce::uint32> (py),
                                                       roli::LEDColour (juce::Colours::white.getARGB()));
                            }
                        }
                    }
                }
            }
        }
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LightpadDevice)
};
