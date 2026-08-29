#pragma once

#include <JuceHeader.h>
#include "ColorUtilities.h"
#include "WfsLookAndFeel.h"

/**
 * Shared colouring for the two "none + 1..N" index dropdowns: the Outputs tab's
 * Array selector and the Inputs tab's Cluster selector.
 *
 * Both are structurally identical — item 1 is a single/none entry, items 2..N+1
 * are 1..N — and both draw from the SAME palette: WfsColorUtilities::getArrayColor(n)
 * is literally getMarkerColor(n, true), which is what clusters use as well. Array 3
 * and Cluster 3 have always been the same colour; showing it on the selectors makes
 * that visible rather than creating it.
 *
 * Nothing here stores a colour. Every colour is still derived from the index, so the
 * tablet's own copy of the same HSL formula cannot drift out of step.
 */
namespace WfsColouredCombo
{
    /** One dropdown row: a rounded colour chip followed by the row's label.

        MUST stay stateless across openings. ComboBox::showPopup() copies the root
        menu every time it opens, and PopupMenu::Item's copy constructor SHARES the
        reference-counted customComponent rather than cloning it — so this exact
        instance is reused on every open, while isTicked is stamped freshly onto the
        copy's Item. Everything painted therefore comes from the constructor
        arguments plus the live isItemHighlighted() / getItem()->isTicked; caching
        anything at first paint would leave a stale tick on the next open.
    */
    class IndexMenuItem : public juce::PopupMenu::CustomComponent
    {
    public:
        IndexMenuItem(juce::ComboBox& ownerIn, juce::Colour chipColourIn, juce::String labelIn)
            : juce::PopupMenu::CustomComponent(true),  // true: a click selects the item
              owner(&ownerIn),
              chipColour(chipColourIn),
              label(std::move(labelIn))
        {
        }

        void getIdealSize(int& idealWidth, int& idealHeight) override
        {
            auto font = menuFont();
            idealWidth = margin() * 2 + chipWidth() + gap()
                       + juce::GlyphArrangement::getStringWidthInt(font, label)
                       + tickColumn();

            // A CUSTOM row is never offered the menu's standardItemHeight — ItemComponent
            // passes it only to getIdealPopupMenuItemSizeWithOptions, on the plain path
            // (juce_PopupMenu.cpp:160-169). ComboBox sets that height to its label's, i.e.
            // getHeight() - 2 (LookAndFeel_V2 :1310), so deriving it from the owner is what
            // keeps these rows the same height as every other dropdown in the app — and,
            // now that the "none" row is custom too, the same as each other.
            const int standard = (owner != nullptr ? owner->getHeight() - 2 : 0);
            idealHeight = standard > 0 ? standard
                                       : juce::jmax(18, juce::roundToInt(font.getHeight() * 1.8f));
        }

        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds();

            // Highlight row: the menu paints its own background for normal rows, so
            // only the hovered row needs filling here.
            if (isItemHighlighted())
            {
                g.setColour(findColour(juce::PopupMenu::highlightedBackgroundColourId));
                g.fillRect(bounds);
            }

            auto area = bounds.reduced(margin(), 0);

            auto chipArea = area.removeFromLeft(chipWidth());
            if (!chipColour.isTransparent())
            {
                const float chipH = juce::jmin(static_cast<float>(chipArea.getWidth()),
                                               chipArea.getHeight() * 0.62f);
                auto chip = chipArea.toFloat().withSizeKeepingCentre(static_cast<float>(chipArea.getWidth()), chipH);
                g.setColour(chipColour);
                g.fillRoundedRectangle(chip, 2.0f);
                // A hairline keeps a pale chip from dissolving into the light theme.
                g.setColour(juce::Colours::black.withAlpha(0.35f));
                g.drawRoundedRectangle(chip.reduced(0.5f), 2.0f, 1.0f);
            }

            area.removeFromLeft(gap());

            // The tick lives in its own column so labels stay left-aligned with it.
            auto tickArea = area.removeFromRight(tickColumn());
            if (auto* item = getItem())
            {
                if (item->isTicked)
                {
                    g.setColour(textColour());
                    auto t = tickArea.toFloat().reduced(tickArea.getWidth() * 0.3f,
                                                        tickArea.getHeight() * 0.34f);
                    juce::Path tick;
                    tick.startNewSubPath(t.getX(), t.getCentreY());
                    tick.lineTo(t.getCentreX() - t.getWidth() * 0.08f, t.getBottom());
                    tick.lineTo(t.getRight(), t.getY());
                    g.strokePath(tick, juce::PathStrokeType(1.6f));
                }
            }

            g.setColour(textColour());
            g.setFont(menuFont());
            g.drawFittedText(label, area, juce::Justification::centredLeft, 1);
        }

    private:
        juce::Font menuFont() const { return getLookAndFeel().getPopupMenuFont(); }

        juce::Colour textColour() const
        {
            return findColour(isItemHighlighted() ? juce::PopupMenu::highlightedTextColourId
                                                  : juce::PopupMenu::textColourId);
        }

        static int sc(float ref) { return juce::jmax(1, juce::roundToInt(ref * WfsLookAndFeel::uiScale)); }
        static int margin()    { return sc(8.0f); }
        static int chipWidth() { return sc(14.0f); }
        static int gap()       { return sc(8.0f); }
        static int tickColumn(){ return sc(18.0f); }

        juce::Component::SafePointer<juce::ComboBox> owner;
        juce::Colour chipColour;
        juce::String label;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IndexMenuItem)
    };

    /** Fills a ComboBox with item 1 = noneText (no chip) and items 2..count+1 =
        "<itemPrefix> n", each carrying its index colour as a chip.

        The item title is passed to addCustomItem, which stores it as Item::text —
        that is what ComboBox::getItemText / setSelectedId / getText() read, so the
        selection round-trip and the TTS announcements keep working unchanged.
        Call once, at construction.
    */
    inline void populate(juce::ComboBox& box,
                         const juce::String& noneText,
                         const juce::String& itemPrefix,
                         int count = 10)
    {
        auto* menu = box.getRootMenu();
        if (menu == nullptr)
            return;

        menu->clear();

        // "Single"/"None" carries a TRANSPARENT chip rather than being a plain item: it is
        // the absence of an index, not index 0, so it must paint no colour — but a plain
        // item would be sized by the menu's standardItemHeight while every custom row sizes
        // itself, leaving row 1 several pixels taller with its label at a different x.
        menu->addCustomItem(1,
                            std::make_unique<IndexMenuItem>(box, juce::Colours::transparentBlack, noneText),
                            nullptr,
                            noneText);

        for (int i = 1; i <= count; ++i)
        {
            const juce::String text = itemPrefix + " " + juce::String(i);
            menu->addCustomItem(i + 1,
                                std::make_unique<IndexMenuItem>(box, WfsColorUtilities::getArrayColor(i), text),
                                nullptr,
                                text);
        }
    }

    /** Tints the closed box for the selected index: 0 = none, 1..N = coloured.

        For "none" the overrides are REMOVED rather than set to a theme value, so the
        control keeps inheriting from WfsLookAndFeel and follows all three themes with
        no listener work.
    */
    inline void applyTint(juce::ComboBox& box, int index)
    {
        if (index <= 0)
        {
            box.removeColour(juce::ComboBox::backgroundColourId);
            box.removeColour(juce::ComboBox::textColourId);
            box.removeColour(juce::ComboBox::arrowColourId);
        }
        else
        {
            // Black, not getContrastingTextColor(): that helper switches at luminance 0.4,
            // but the break-even point where black and white contrast equally is 0.179, so
            // it hands white to indices 6-10 (luminance 0.27-0.38) at 2.3-3.0:1 where black
            // would give 6.3-8.6:1. All ten hues pass WCAG AA on black and none do on white.
            // This also matches what OutputsTab already hardcodes for the array tiles.
            const auto bg = WfsColorUtilities::getArrayColor(index);

            box.setColour(juce::ComboBox::backgroundColourId, bg);
            box.setColour(juce::ComboBox::textColourId, juce::Colours::black);
            box.setColour(juce::ComboBox::arrowColourId, juce::Colours::black.withAlpha(0.8f));
        }
        // No repaint() needed: setColour/removeColour both go through
        // ComboBox::colourChanged(), which repaints and refreshes its internal label.
    }
}
