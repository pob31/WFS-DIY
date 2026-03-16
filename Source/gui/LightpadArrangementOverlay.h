/*
  ==============================================================================
    LightpadArrangementOverlay.h
    Popup overlay showing the detected Lightpad arrangement with click-to-split.
    Used in SystemConfigTab for visual pad arrangement and split configuration.
    Follows the ChannelSelectorOverlay / ChannelSelectorBackdrop pattern.
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Controllers/Sampler/LightpadTypes.h"
#include "LightpadZoneOverlay.h"  // for LightpadZoneBackdrop
#include "ColorScheme.h"
#include "../Localization/LocalizationManager.h"

/**
 * Lightpad Arrangement Overlay
 * Shows the physical pad arrangement with large squares.
 * Click a pad to toggle its split state.
 */
class LightpadArrangementOverlay : public juce::Component
{
public:
    LightpadArrangementOverlay (const std::vector<PadLayoutInfo>& padLayouts,
                                 std::function<void (int padIndex, bool split)> onSplitToggled,
                                 std::function<void()> onDismiss)
        : layouts (padLayouts),
          onSplit (std::move (onSplitToggled)),
          onClose (std::move (onDismiss))
    {
        setOpaque (false);
        setAlwaysOnTop (true);
    }

    juce::Point<int> getRequiredSize() const
    {
        auto [gridW, gridH] = getGridDimensions();
        int w = padding * 2 + gridW * cellSize + (gridW - 1) * gap;
        int h = titleHeight + gridH * cellSize + (gridH - 1) * gap + padding;
        return { juce::jmax (w, 200), h };
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
        g.drawText (LOC ("systemConfig.labels.lightpadArrangement"),
                    padding, padding, getWidth() - padding * 2 - 30, titleHeight - padding,
                    juce::Justification::centredLeft);

        // Close button
        closeButtonBounds = juce::Rectangle<int> (getWidth() - padding - 24, padding, 24, 20);
        g.setColour (juce::Colour (0xFF555555));
        g.fillRoundedRectangle (closeButtonBounds.toFloat(), 4.0f);
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.setFont (juce::FontOptions().withHeight (12.0f).withStyle ("Bold"));
        g.drawText ("X", closeButtonBounds, juce::Justification::centred);

        // Pad grid
        if (layouts.empty()) return;

        auto grid = getNormalizedGrid();
        int totalW = grid.gridW * cellSize + (grid.gridW - 1) * gap;
        int startX = padding + (getWidth() - padding * 2 - totalW) / 2;
        int startY = titleHeight;

        padBounds.clear();

        for (int i = 0; i < static_cast<int> (layouts.size()); ++i)
        {
            auto& pl = layouts[i];
            auto [col, row] = grid.padPositions[i];
            auto rect = juce::Rectangle<int> (
                startX + col * (cellSize + gap),
                startY + row * (cellSize + gap),
                cellSize, cellSize);

            padBounds.push_back ({ i, rect });

            auto colour = LightpadColours::getPadColour (pl.padIndex);

            // Fill
            g.setColour (colour.withAlpha (0.85f));
            g.fillRoundedRectangle (rect.toFloat(), 6.0f);
            g.setColour (colour);
            g.drawRoundedRectangle (rect.toFloat(), 6.0f, 1.5f);

            // Split cross
            if (pl.isSplit)
            {
                g.setColour (juce::Colours::black.withAlpha (0.7f));
                float cx = static_cast<float> (rect.getCentreX());
                float cy = static_cast<float> (rect.getCentreY());
                g.drawLine (cx, static_cast<float> (rect.getY() + 4),
                            cx, static_cast<float> (rect.getBottom() - 4), 2.5f);
                g.drawLine (static_cast<float> (rect.getX() + 4), cy,
                            static_cast<float> (rect.getRight() - 4), cy, 2.5f);
            }

            // USB label on master
            if (pl.isMaster)
            {
                g.setColour (juce::Colours::white.withAlpha (0.85f));
                g.setFont (juce::FontOptions().withHeight (10.0f));
                auto usbArea = juce::Rectangle<int> (rect.getX(), rect.getY(),
                                                      rect.getWidth(), 14);
                g.drawText ("USB", usbArea, juce::Justification::centred);
            }

            // Pad number
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.setFont (juce::FontOptions().withHeight (18.0f));
            g.drawText (juce::String (pl.padIndex + 1), rect, juce::Justification::centred);

            // Split hint at bottom
            g.setColour (juce::Colours::white.withAlpha (0.4f));
            g.setFont (juce::FontOptions().withHeight (9.0f));
            auto hintArea = juce::Rectangle<int> (rect.getX(), rect.getBottom() - 14,
                                                   rect.getWidth(), 14);
            g.drawText (pl.isSplit ? LOC ("systemConfig.labels.split")
                                   : LOC ("systemConfig.labels.clickToSplit"),
                        hintArea, juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto pos = e.getPosition();

        // Close button
        if (closeButtonBounds.contains (pos))
        {
            if (onClose) onClose();
            return;
        }

        // Pad click → toggle split
        for (auto& [idx, rect] : padBounds)
        {
            if (rect.contains (pos) && idx < static_cast<int> (layouts.size()))
            {
                layouts[idx].isSplit = ! layouts[idx].isSplit;

                if (onSplit)
                    onSplit (layouts[idx].padIndex, layouts[idx].isSplit);

                repaint();
                return;
            }
        }
    }

private:
    std::vector<PadLayoutInfo> layouts;
    std::function<void (int, bool)> onSplit;
    std::function<void()> onClose;

    std::vector<std::pair<int, juce::Rectangle<int>>> padBounds;
    juce::Rectangle<int> closeButtonBounds;

    static constexpr int cellSize = 100;
    static constexpr int gap = 2;
    static constexpr int padding = 12;
    static constexpr int titleHeight = 32;

    struct NormalizedGrid
    {
        int gridW = 1, gridH = 1;
        std::vector<std::pair<int, int>> padPositions;
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LightpadArrangementOverlay)
};
