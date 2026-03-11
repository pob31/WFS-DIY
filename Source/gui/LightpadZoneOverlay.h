/*
  ==============================================================================
    LightpadZoneOverlay.h
    Popup overlay showing the detected Lightpad arrangement as clickable zones.
    Used in SamplerSubTab for visual zone-to-input assignment.
    Follows the ChannelSelectorOverlay / ChannelSelectorBackdrop pattern.
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Lightpad/LightpadTypes.h"
#include "ColorScheme.h"
#include "WfsLookAndFeel.h"
#include "../Localization/LocalizationManager.h"

/**
 * Transparent backdrop for click-outside-to-dismiss behavior.
 * (Shared pattern with ChannelSelectorBackdrop.)
 */
class LightpadZoneBackdrop : public juce::Component
{
public:
    LightpadZoneBackdrop (std::function<void()> onClickOutside)
        : onClick (std::move (onClickOutside))
    {
        setOpaque (false);
        setInterceptsMouseClicks (true, false);
    }

    void paint (juce::Graphics&) override {}

    void mouseDown (const juce::MouseEvent&) override
    {
        if (onClick)
            onClick();
    }

private:
    std::function<void()> onClick;
};

/**
 * Lightpad Zone Overlay
 * Shows the physical pad arrangement with clickable zones.
 * Split pads show 4 quadrants; unsplit pads are a single zone.
 * Already-assigned zones are dimmed; current input's zone is highlighted.
 */
class LightpadZoneOverlay : public juce::Component
{
public:
    /**
     * @param padLayouts     Current topology layout info
     * @param allZones       All active zone IDs with display names
     * @param assignedZones  Map of zoneId → inputIndex (0-based) for all assigned zones
     * @param currentInputIdx The current input index (0-based) being edited
     * @param currentZoneId  Zone currently assigned to this input (-1 if none)
     * @param onZoneSelected Callback: fires with selected zoneId (-1 for none)
     */
    LightpadZoneOverlay (const std::vector<PadLayoutInfo>& padLayouts,
                         const std::vector<std::pair<int, juce::String>>& allZones,
                         const std::map<int, int>& assignedZones,
                         int currentInputIdx,
                         int currentZoneId,
                         std::function<void (int zoneId)> onZoneSelected)
        : layouts (padLayouts),
          zones (allZones),
          assigned (assignedZones),
          currentInput (currentInputIdx),
          selectedZone (currentZoneId),
          onSelect (std::move (onZoneSelected))
    {
        setOpaque (false);
        setAlwaysOnTop (true);

        // Build zone ID → name map
        for (auto& [id, name] : zones)
            zoneNames[id] = name;

        // Build zone ID → owner input map (only zones owned by OTHER inputs)
        for (auto& [zid, inputIdx] : assigned)
            if (inputIdx != currentInput)
                otherAssigned.insert (zid);
    }

    juce::Point<int> getRequiredSize() const
    {
        auto [gridW, gridH] = getGridDimensions();
        int w = padding * 2 + gridW * cellSize + (gridW - 1) * gap;
        int h = titleHeight + noneRowHeight + noneGap + gridH * cellSize + (gridH - 1) * gap + padding;
        return { juce::jmax (w, 180), h };
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Drop shadow
        juce::DropShadow shadow (juce::Colours::black.withAlpha (0.5f), 8, { 2, 2 });
        shadow.drawForRectangle (g, getLocalBounds());

        // Background
        g.setColour (juce::Colour (0xFF2A2A2A));
        g.fillRoundedRectangle (bounds, 8.0f);
        g.setColour (juce::Colour (0xFF505050));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, 1.0f);

        // Title
        g.setColour (ColorScheme::get().textPrimary);
        g.setFont (juce::FontOptions().withHeight (14.0f).withStyle ("Bold"));
        g.drawText (LOC ("sampler.labels.selectZone"),
                    padding, padding, getWidth() - padding * 2 - 30, titleHeight - padding,
                    juce::Justification::centredLeft);

        // Close button area
        auto closeBounds = juce::Rectangle<int> (getWidth() - padding - 24, padding, 24, 20);
        g.setColour (juce::Colour (0xFF555555));
        g.fillRoundedRectangle (closeBounds.toFloat(), 4.0f);
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.setFont (juce::FontOptions().withHeight (12.0f).withStyle ("Bold"));
        g.drawText ("X", closeBounds, juce::Justification::centred);
        closeButtonBounds = closeBounds;

        // "None" row
        int noneY = titleHeight;
        auto noneRect = juce::Rectangle<int> (padding, noneY, getWidth() - padding * 2, noneRowHeight - 4);
        bool noneSelected = (selectedZone < 0);
        g.setColour (noneSelected ? juce::Colour (0xFF4080FF) : juce::Colour (0xFF3A3A3A));
        g.fillRoundedRectangle (noneRect.toFloat(), 4.0f);
        if (noneSelected)
        {
            g.setColour (juce::Colours::white);
            g.drawRoundedRectangle (noneRect.toFloat(), 4.0f, 2.0f);
        }
        g.setColour (juce::Colours::white.withAlpha (noneSelected ? 1.0f : 0.7f));
        g.setFont (juce::FontOptions().withHeight (13.0f));
        g.drawText (LOC ("sampler.lightpadZone.none"), noneRect, juce::Justification::centred);
        noneButtonBounds = noneRect;

        // Pad grid
        if (layouts.empty())
            return;

        auto grid = getNormalizedGrid();
        int gridStartY = titleHeight + noneRowHeight + noneGap;
        int totalW = grid.gridW * cellSize + (grid.gridW - 1) * gap;
        int startX = padding + (getWidth() - padding * 2 - totalW) / 2;
        int startY = gridStartY;

        zoneBounds.clear();

        for (int pi = 0; pi < static_cast<int> (layouts.size()); ++pi)
        {
            auto& pl = layouts[pi];
            auto [col, row] = grid.padPositions[pi];
            auto padRect = juce::Rectangle<int> (
                startX + col * (cellSize + gap),
                startY + row * (cellSize + gap),
                cellSize, cellSize);

            auto padColour = LightpadColours::getPadColour (pl.padIndex);

            if (pl.isSplit)
            {
                // Draw 4 quadrants
                int halfW = padRect.getWidth() / 2 - 1;
                int halfH = padRect.getHeight() / 2 - 1;

                for (int q = 0; q < 4; ++q)
                {
                    int qx = padRect.getX() + (q % 2 == 0 ? 0 : halfW + 2);
                    int qy = padRect.getY() + (q < 2 ? 0 : halfH + 2);
                    auto qRect = juce::Rectangle<int> (qx, qy, halfW, halfH);
                    int zid = encodeZoneId (pl.padIndex, q);

                    drawZoneButton (g, qRect, zid, padColour, 10.0f);
                    zoneBounds.push_back ({ zid, qRect });
                }
            }
            else
            {
                int zid = encodeZoneId (pl.padIndex, 0);
                drawZoneButton (g, padRect, zid, padColour, 13.0f);
                zoneBounds.push_back ({ zid, padRect });
            }
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto pos = e.getPosition();

        // Close button
        if (closeButtonBounds.contains (pos))
        {
            if (onSelect)
                onSelect (selectedZone);  // no change
            return;
        }

        // None button
        if (noneButtonBounds.contains (pos))
        {
            if (onSelect)
                onSelect (-1);
            return;
        }

        // Zone buttons
        for (auto& [zid, rect] : zoneBounds)
        {
            if (rect.contains (pos) && otherAssigned.count (zid) == 0)
            {
                if (onSelect)
                    onSelect (zid);
                return;
            }
        }
    }

private:
    std::vector<PadLayoutInfo> layouts;
    std::vector<std::pair<int, juce::String>> zones;
    std::map<int, int> assigned;
    int currentInput;
    int selectedZone;
    std::function<void (int)> onSelect;

    std::map<int, juce::String> zoneNames;
    std::set<int> otherAssigned;

    // Hit-test storage (rebuilt each paint)
    std::vector<std::pair<int, juce::Rectangle<int>>> zoneBounds;
    juce::Rectangle<int> noneButtonBounds;
    juce::Rectangle<int> closeButtonBounds;

    static constexpr int cellSize = 100;
    static constexpr int gap = 2;
    static constexpr int padding = 12;
    static constexpr int titleHeight = 32;
    static constexpr int noneRowHeight = 30;
    static constexpr int noneGap = 6;

    // Normalize layout positions to contiguous 0-based indices
    // Returns { gridW, gridH, vector of (col, row) per pad }
    struct NormalizedGrid
    {
        int gridW = 1, gridH = 1;
        std::vector<std::pair<int, int>> padPositions;  // (col, row) per layout entry
    };

    NormalizedGrid getNormalizedGrid() const
    {
        NormalizedGrid result;
        if (layouts.empty()) return result;

        std::set<int> uniqueX, uniqueY;
        for (auto& pl : layouts)
        {
            uniqueX.insert (pl.layoutX);
            uniqueY.insert (pl.layoutY);
        }

        // Map raw coords to contiguous indices
        std::map<int, int> xMap, yMap;
        int idx = 0;
        for (int x : uniqueX) xMap[x] = idx++;
        idx = 0;
        for (int y : uniqueY) yMap[y] = idx++;

        result.gridW = static_cast<int> (uniqueX.size());
        result.gridH = static_cast<int> (uniqueY.size());

        for (auto& pl : layouts)
            result.padPositions.push_back ({ xMap[pl.layoutX], yMap[pl.layoutY] });

        return result;
    }

    std::pair<int, int> getGridDimensions() const
    {
        auto grid = getNormalizedGrid();
        return { grid.gridW, grid.gridH };
    }

    void drawZoneButton (juce::Graphics& g, juce::Rectangle<int> rect, int zoneId,
                          juce::Colour padColour, float fontSize)
    {
        bool isSelected = (zoneId == selectedZone);
        bool isDisabled = (otherAssigned.count (zoneId) > 0);

        // Background
        juce::Colour bgColour = isDisabled
            ? padColour.withAlpha (0.15f)
            : (isSelected ? padColour : padColour.withAlpha (0.6f));

        g.setColour (bgColour);
        g.fillRoundedRectangle (rect.toFloat(), 4.0f);

        // Bright border for selected zone
        if (isSelected)
        {
            g.setColour (juce::Colours::white);
            g.drawRoundedRectangle (rect.toFloat(), 4.0f, 2.0f);
        }
        else
        {
            g.setColour (padColour.withAlpha (0.5f));
            g.drawRoundedRectangle (rect.toFloat(), 4.0f, 1.0f);
        }

        // Zone name
        auto it = zoneNames.find (zoneId);
        juce::String label = (it != zoneNames.end()) ? it->second : "?";

        g.setColour (isDisabled ? juce::Colours::white.withAlpha (0.3f)
                                : juce::Colours::white.withAlpha (0.9f));
        g.setFont (juce::FontOptions().withHeight (fontSize));
        g.drawText (label, rect, juce::Justification::centred);

        // Show owner input number for disabled zones
        if (isDisabled)
        {
            auto ownerIt = assigned.find (zoneId);
            if (ownerIt != assigned.end())
            {
                g.setColour (juce::Colours::white.withAlpha (0.4f));
                g.setFont (juce::FontOptions().withHeight (9.0f));
                g.drawText ("In " + juce::String (ownerIt->second + 1),
                            rect.removeFromBottom (12), juce::Justification::centred);
            }
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LightpadZoneOverlay)
};
