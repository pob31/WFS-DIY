#pragma once

#include <JuceHeader.h>
#include <vector>
#include "ColorUtilities.h"
#include "ColorScheme.h"
#include "WfsLookAndFeel.h"
#include "../Localization/LocalizationManager.h"

// Forward declaration
class ChannelSelectorOverlay;

/**
 * Small circular close button: filled disc with an X stroked through it.
 */
class CircularCloseButton : public juce::Button
{
public:
    CircularCloseButton() : juce::Button("close") {}

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        const float r  = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;

        juce::Colour bg(0xFF505050);
        if (isButtonDown)      bg = bg.darker(0.2f);
        else if (isMouseOver)  bg = bg.brighter(0.3f);

        g.setColour(bg);
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);

        const float xOff = r * 0.42f;
        const float thickness = juce::jmax(1.0f, r * 0.22f);
        g.setColour(juce::Colours::white);
        g.drawLine(cx - xOff, cy - xOff, cx + xOff, cy + xOff, thickness);
        g.drawLine(cx - xOff, cy + xOff, cx + xOff, cy - xOff, thickness);
    }
};

/**
 * Round colour swatch shown in the overlay's title bar, left of the close button.
 *
 * Only appears when the owner installs a colour editor — so the Outputs and Reverb
 * selectors, which do not, are untouched. It is deliberately in the popup rather than
 * beside the collapsed button: the collapsed button already shows the same colour, and
 * inside the grid you can see every other channel's colour while choosing.
 */
class ChannelColourButton : public juce::Button
{
public:
    ChannelColourButton() : juce::Button("colour") {}

    /** Colour to paint — asked for on every repaint, so it cannot go stale. */
    std::function<juce::Colour()> getColour;

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        const float r  = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;

        juce::Colour fill = getColour ? getColour() : juce::Colour(0xFF808080);
        if (isButtonDown)      fill = fill.darker(0.2f);
        else if (isMouseOver)  fill = fill.brighter(0.2f);

        g.setColour(fill);
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);

        // Ring, so a swatch whose colour matches the card behind it is still findable.
        g.setColour(isMouseOver ? juce::Colours::white : juce::Colour(0xFF909090));
        g.drawEllipse(cx - r + 0.75f, cy - r + 0.75f, r * 2.0f - 1.5f, r * 2.0f - 1.5f, 1.5f);
    }
};

/**
 * Transparent backdrop for click-outside-to-dismiss behavior
 */
class ChannelSelectorBackdrop : public juce::Component
{
public:
    ChannelSelectorBackdrop(std::function<void()> onClickOutside)
        : onClick(std::move(onClickOutside))
    {
        setOpaque(false);
        setInterceptsMouseClicks(true, false);
    }

    void paint(juce::Graphics&) override {}

    void mouseDown(const juce::MouseEvent&) override
    {
        if (onClick)
            onClick();
    }

private:
    std::function<void()> onClick;
};

/**
 * Channel Selector Overlay Component
 * A reusable grid-based channel selector that opens as an overlay.
 * Used for Input, Output, Cluster, and Reverb tabs.
 */
class ChannelSelectorOverlay : public juce::Component
{
public:
    ChannelSelectorOverlay(int numChannels, int currentChannel, std::function<void(int)> onChannelSelected,
                          std::function<juce::Colour(int)> channelColorProvider = nullptr,
                          std::function<juce::String(int)> channelNameProvider = nullptr,
                          std::function<juce::Colour(int)> textColorProvider = nullptr,
                          std::vector<int> channelIdsIn = {},
                          std::function<bool(int)> channelStereoProvider = nullptr,
                          std::function<void(int, juce::Component&)> colourEditor = nullptr)
        : totalChannels(numChannels),
          selectedChannel(currentChannel),
          onSelect(std::move(onChannelSelected)),
          getChannelColor(std::move(channelColorProvider)),
          getChannelName(std::move(channelNameProvider)),
          getTextColor(std::move(textColorProvider)),
          channelIds(std::move(channelIdsIn)),
          getChannelStereo(std::move(channelStereoProvider)),
          onEditColour(std::move(colourEditor))
    {
        setOpaque(false);
        setAlwaysOnTop(true);

        // Only present when the owner installs an editor, so selectors that have no
        // per-channel colour to edit (Outputs, Reverb) render exactly as before.
        addChildComponent(colourButton);
        if (onEditColour)
        {
            colourButton.getColour = [this]() -> juce::Colour {
                return getChannelColor ? getChannelColor(selectedChannel) : juce::Colour(0xFF808080);
            };
            colourButton.onClick = [this]() {
                if (onEditColour)
                    onEditColour(selectedChannel, colourButton);
            };
            // The tab's helpTextMap only covers components inside the tab, and this one lives
            // in a popup parented to the window — so it explains itself with a tooltip
            // instead, through the app-wide TooltipWindow.
            colourButton.setTooltip(LOC("inputs.help.colourSwatch"));
            colourButton.setVisible(true);
        }

        // Classical stereo mark in unit space: circles of radius 1 centred at
        // (1, 1) and (2.5, 1) — centre separation 1.5 x radius. Stroked in
        // paintOverChildren, never filled: a filled pair reads as a meter.
        stereoGlyph.addEllipse(0.0f, 0.0f, 2.0f, 2.0f);
        stereoGlyph.addEllipse(1.5f, 0.0f, 2.0f, 2.0f);

        // Dense fallback: ids are 1..numChannels. Inputs pass their explicit
        // live channel-number list instead (stable numbers, gaps possible) —
        // the grid shows one tile per LIVE channel labeled by its number.
        if (channelIds.empty())
            for (int i = 1; i <= totalChannels; ++i)
                channelIds.push_back(i);
        totalChannels = static_cast<int>(channelIds.size());

        // Calculate adaptive grid dimensions - favor rows over columns
        // since buttons are wider (60) than tall (40)
        calculateGridDimensions(totalChannels);

        // Create channel buttons
        for (int channelId : channelIds)
        {
            // Get button text - show name if available, otherwise just number
            juce::String buttonText;
            if (getChannelName)
            {
                juce::String name = getChannelName(channelId);
                if (name.isNotEmpty())
                    buttonText = juce::String(channelId) + "\n" + name;
                else
                    buttonText = juce::String(channelId);
            }
            else
            {
                buttonText = juce::String(channelId);
            }

            auto* btn = new juce::TextButton(buttonText);
            btn->setClickingTogglesState(false);

            // The badge is painted over the tile, so it never reaches the
            // accessible name (which JUCE derives from the button text) —
            // without this a screen reader cannot tell stereo from mono.
            if (getChannelStereo && getChannelStereo(channelId))
                btn->setDescription(LOC("systemConfig.channelList.stereo"));

            btn->onClick = [this, channelId]() {
                if (onSelect)
                    onSelect(channelId);
            };
            channelButtons.add(btn);
            addAndMakeVisible(btn);
        }

        // Close button (circular icon)
        closeButton.onClick = [this]() {
            if (onSelect)
                onSelect(selectedChannel); // Return current selection (no change)
        };
        addAndMakeVisible(closeButton);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Draw drop shadow
        juce::DropShadow shadow(juce::Colours::black.withAlpha(0.5f), 8, {2, 2});
        shadow.drawForRectangle(g, getLocalBounds());

        // Solid background with rounded corners
        g.setColour(ColorScheme::get().surfaceCard);
        g.fillRoundedRectangle(bounds, 8.0f);

        // Border
        g.setColour(juce::Colour(0xFF505050));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);

        // Draw title
        g.setColour(ColorScheme::get().textPrimary);
        g.setFont(juce::FontOptions().withHeight(juce::jmax(10.0f, 14.0f * WfsLookAndFeel::uiScale)).withStyle("Bold"));
        const int titleReserve = sc(30) + (colourButton.isVisible() ? sc(28) : 0);
        g.drawText(LOC("inputs.dialogs.selectChannel"), padding, padding,
                   getWidth() - padding * 2 - titleReserve, titleHeight - padding,
                   juce::Justification::centredLeft);
    }

    void resized() override
    {
        // Close button in top-right (square circular icon, vertically centred in title row)
        const int closeSize = sc(20);
        const int closeY = padding + (titleHeight - padding - closeSize) / 2;
        closeButton.setBounds(getWidth() - padding - closeSize,
                              juce::jmax(closeY, padding),
                              closeSize, closeSize);

        // Colour swatch immediately left of it, same size and baseline.
        if (colourButton.isVisible())
            colourButton.setBounds(getWidth() - padding - closeSize - sc(8) - closeSize,
                                   juce::jmax(closeY, padding),
                                   closeSize, closeSize);

        // Position buttons in grid below title
        const int startX = padding;
        const int startY = titleHeight + topRowExtraPadding;

        for (int i = 0; i < channelButtons.size(); ++i)
        {
            int row = i / numColumns;
            int col = i % numColumns;
            int x = startX + col * (buttonWidth + spacing);
            int y = startY + row * (buttonHeight + spacing);

            channelButtons[i]->setBounds(x, y, buttonWidth, buttonHeight);

            const int channelId = channelIds[static_cast<size_t>(i)];

            // Get color for this channel
            juce::Colour buttonColor;
            if (getChannelColor)
            {
                // Use custom color from provider
                buttonColor = getChannelColor(channelId);

                // If this is the selected channel, brighten it slightly
                if (channelId == selectedChannel)
                    buttonColor = buttonColor.brighter(0.3f);
            }
            else
            {
                // Default color scheme
                if (channelId == selectedChannel)
                    buttonColor = juce::Colour(0xFF4080FF);
                else
                    buttonColor = juce::Colour(0xFF3A3A3A);
            }

            // Get text color for this channel
            juce::Colour textColor = juce::Colours::white;  // Default to white for dark buttons
            if (getTextColor)
                textColor = getTextColor(channelId);

            channelButtons[i]->setColour(juce::TextButton::buttonColourId, buttonColor);
            channelButtons[i]->setColour(juce::TextButton::textColourOffId, textColor);
        }

        updateStereoBadges();
    }

    /** Re-asks the colour providers for every tile, without re-laying anything out.

        Needed because a colour can now be changed while the grid is open: the tiles are
        coloured in resized(), which does not run again on its own. */
    void refreshTileColours()
    {
        for (int i = 0; i < channelButtons.size(); ++i)
        {
            const int channelId = channelIds[static_cast<size_t>(i)];

            juce::Colour buttonColor;
            if (getChannelColor)
            {
                buttonColor = getChannelColor(channelId);
                if (channelId == selectedChannel)
                    buttonColor = buttonColor.brighter(0.3f);
            }
            else
            {
                buttonColor = (channelId == selectedChannel) ? juce::Colour(0xFF4080FF)
                                                             : juce::Colour(0xFF3A3A3A);
            }

            juce::Colour textColor = juce::Colours::white;
            if (getTextColor)
                textColor = getTextColor(channelId);

            channelButtons[i]->setColour(juce::TextButton::buttonColourId, buttonColor);
            channelButtons[i]->setColour(juce::TextButton::textColourOffId, textColor);
        }

        colourButton.repaint();
        repaint();
    }

    /** Stereo mark, stamped over the tiles rather than baked into their text:
        the number is drawn centred by drawButtonText and must stay exactly
        where it is, so the mark can only live beside it. With no stereo
        provider updateStereoBadges() leaves the list empty and nothing is
        drawn, which is what leaves the Outputs and Reverb selectors untouched. */
    void paintOverChildren(juce::Graphics& g) override
    {
        for (size_t i = 0; i < stereoBadgeBounds.size(); ++i)
        {
            const auto badge = stereoBadgeBounds[i];
            if (badge.isEmpty())
                continue;

            const int channelId = channelIds[i];
            g.setColour(getTextColor ? getTextColor(channelId)
                                     : juce::Colours::white);

            juce::Path scaled(stereoGlyph);
            scaled.applyTransform(stereoGlyph.getTransformToScaleToFit(badge, true));
            g.strokePath(scaled, juce::PathStrokeType(juce::jmax(1.0f, badge.getHeight() * 0.10f),
                                                      juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        }
    }

    // Get the required size for this overlay based on channel count
    juce::Point<int> getRequiredSize() const
    {
        int width = padding * 2 + numColumns * buttonWidth + (numColumns - 1) * spacing;
        int height = titleHeight + topRowExtraPadding + numRows * buttonHeight + (numRows - 1) * spacing + padding;
        return {width, height};
    }

private:
    /** Where each tile's stereo mark goes, in overlay coordinates; an empty
        rectangle means no mark. Measured here and not in paintOverChildren
        because it depends only on tile size and text, both settled by the
        time layout ends — measuring per repaint would re-shape every tile's
        text on every hover. */
    void updateStereoBadges()
    {
        stereoBadgeBounds.assign(static_cast<size_t>(channelButtons.size()), {});

        const auto unitBounds = stereoGlyph.getBounds();
        if (!getChannelStereo || unitBounds.getHeight() <= 0.0f)
            return;

        const float aspect = unitBounds.getWidth() / unitBounds.getHeight();

        for (int i = 0; i < channelButtons.size(); ++i)
        {
            if (!getChannelStereo(channelIds[static_cast<size_t>(i)]))
                continue;

            auto* btn = channelButtons[i];
            juce::Rectangle<float> number;
            if (!getTopLineInkBounds(*btn, number))
                continue;

            // WfsLookAndFeel::drawButtonBackground trims cornerSize = 6 px off
            // each side, and half the stroke bleeds outside the path bounds —
            // past this the mark would sit on the overlay, not on the tile.
            const float surfaceRight = static_cast<float>(btn->getWidth()) - 7.0f;
            const float gap = juce::jmax(2.0f, number.getHeight() * 0.30f);
            const float room = surfaceRight - (number.getRight() + gap);

            // A tile too narrow for both loses the mark, never the number: the
            // accessible description still reports the channel as stereo.
            if (room <= 1.0f)
                continue;

            float badgeH = number.getHeight();
            float badgeW = badgeH * aspect;
            if (badgeW > room)
            {
                badgeW = room;
                badgeH = badgeW / aspect;
            }

            juce::Rectangle<float> badge(number.getRight() + gap,
                                         number.getCentreY() - badgeH * 0.5f,
                                         badgeW, badgeH);
            badge.translate(static_cast<float>(btn->getX()), static_cast<float>(btn->getY()));
            stereoBadgeBounds[static_cast<size_t>(i)] = badge;
        }
    }

    /** Ink bounds of the tile's FIRST text line (the channel number), in the
        button's own coordinates. Reproduces LookAndFeel_V2::drawButtonText —
        same font, same yIndent / leftIndent / rightIndent, same two-line
        centred fit — so the badge tracks where JUCE actually put the number,
        including the horizontal squash drawFittedText applies when a long
        name shares the tile. Measured from the glyph outlines rather than the
        font metrics: ascent and descent would drag the badge off the digits.
        Returns false when the top line has no outline to measure. */
    static bool getTopLineInkBounds(juce::TextButton& btn, juce::Rectangle<float>& bounds)
    {
        const juce::Font font = btn.getLookAndFeel().getTextButtonFont(btn, btn.getHeight());

        const int yIndent = juce::jmin(4, btn.proportionOfHeight(0.3f));
        const int cornerSize = juce::jmin(btn.getHeight(), btn.getWidth()) / 2;
        const int fontHeight = juce::roundToInt(font.getHeight() * 0.6f);
        const int leftIndent  = juce::jmin(fontHeight, 2 + cornerSize / (btn.isConnectedOnLeft() ? 4 : 2));
        const int rightIndent = juce::jmin(fontHeight, 2 + cornerSize / (btn.isConnectedOnRight() ? 4 : 2));
        const int textWidth = btn.getWidth() - leftIndent - rightIndent;

        if (textWidth <= 0)
            return false;

        juce::GlyphArrangement arrangement;
        arrangement.addFittedText(font, btn.getButtonText(),
                                  static_cast<float>(leftIndent), static_cast<float>(yIndent),
                                  static_cast<float>(textWidth),
                                  static_cast<float>(btn.getHeight() - yIndent * 2),
                                  juce::Justification::centred, 2);

        // The top line is the one sitting on the highest baseline; everything
        // below it is the channel name, which the badge must clear.
        float topBaseline = 0.0f;
        bool found = false;
        for (int i = 0; i < arrangement.getNumGlyphs(); ++i)
        {
            const auto& glyph = arrangement.getGlyph(i);
            if (glyph.isWhitespace())
                continue;
            if (!found || glyph.getBaselineY() < topBaseline)
            {
                topBaseline = glyph.getBaselineY();
                found = true;
            }
        }

        if (!found)
            return false;

        juce::Path ink;
        for (int i = 0; i < arrangement.getNumGlyphs(); ++i)
        {
            const auto& glyph = arrangement.getGlyph(i);
            if (glyph.getBaselineY() <= topBaseline + 0.5f)
                glyph.createPath(ink);
        }

        if (ink.isEmpty())
            return false;

        bounds = ink.getBounds();
        return true;
    }

    void calculateGridDimensions(int total)
    {
        // Adaptive grid: favor rows over columns since buttons are wider than tall
        // This creates roughly square panels
        if (total <= 2)
        {
            numColumns = 1;
        }
        else if (total <= 4)
        {
            numColumns = 2;
        }
        else if (total <= 6)
        {
            numColumns = 2;
        }
        else if (total <= 9)
        {
            numColumns = 3;
        }
        else if (total <= 12)
        {
            numColumns = 3;
        }
        else if (total <= 16)
        {
            numColumns = 4;
        }
        else if (total <= 20)
        {
            numColumns = 4;
        }
        else if (total <= 25)
        {
            numColumns = 5;
        }
        else if (total <= 30)
        {
            numColumns = 5;
        }
        else if (total <= 36)
        {
            numColumns = 6;
        }
        else if (total <= 48)
        {
            numColumns = 6;
        }
        else
        {
            numColumns = 8;  // Max 8 columns for larger counts
        }

        numRows = (total + numColumns - 1) / numColumns;
    }

    // Layout constants — scaled by global UI scale
    static int sc(int ref) { float s = WfsLookAndFeel::uiScale; return juce::jmax(static_cast<int>(ref * 0.65f), static_cast<int>(ref * s)); }
    int buttonWidth = sc(90);
    int buttonHeight = sc(54);
    int spacing = sc(4);
    int padding = sc(12);
    int titleHeight = sc(32);
    int topRowExtraPadding = sc(12);

    int totalChannels;
    int selectedChannel;
    int numColumns = 2;
    int numRows = 1;
    std::function<void(int)> onSelect;
    std::function<juce::Colour(int)> getChannelColor;
    std::function<juce::String(int)> getChannelName;
    std::function<juce::Colour(int)> getTextColor;
    std::vector<int> channelIds;   // ids in display order (dense 1..N or live numbers)
    std::function<bool(int)> getChannelStereo;   // null = no marks anywhere
    juce::Path stereoGlyph;        // unit-space mark, fitted per tile
    std::vector<juce::Rectangle<float>> stereoBadgeBounds;   // parallel to channelButtons

    juce::OwnedArray<juce::TextButton> channelButtons;
    CircularCloseButton closeButton;
    ChannelColourButton colourButton;
    std::function<void(int, juce::Component&)> onEditColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelSelectorOverlay)
};

/**
 * Channel Selector Button
 * A button that displays the current channel and opens the selector overlay when clicked.
 */
class ChannelSelectorButton : public juce::Component
{
public:
    ChannelSelectorButton(const juce::String& prefix = "Channel")
        : labelPrefix(prefix)
    {
        setWantsKeyboardFocus(true);  // Allow this component to receive keyboard focus
        selectorButton.onClick = [this]() { showOverlay(); };
        updateButtonText();
        addAndMakeVisible(selectorButton);
    }

    /** Set a custom color provider function for channel buttons.
     *  The function receives a channel number (1-based) and returns a color.
     */
    void setChannelColorProvider(std::function<juce::Colour(int)> provider)
    {
        channelColorProvider = std::move(provider);
        applyTint();
    }

    /** Set a custom name provider function for channel buttons.
     *  The function receives a channel number (1-based) and returns a name string.
     */
    void setChannelNameProvider(std::function<juce::String(int)> provider)
    {
        channelNameProvider = std::move(provider);
    }

    /** Set a custom text color provider function for channel buttons.
     *  The function receives a channel number (1-based) and returns a text color.
     */
    void setTextColorProvider(std::function<juce::Colour(int)> provider)
    {
        textColorProvider = std::move(provider);
        applyTint();
    }

    /** Set an optional stereo predicate for channel tiles.
     *  The function receives a channel number (1-based) and returns true when
     *  that channel is a stereo pair; those tiles get the stereo mark beside
     *  their number. Leaving it null draws no marks at all, so selectors that
     *  have no mono/stereo distinction render exactly as before.
     */
    void setChannelStereoProvider(std::function<bool(int)> provider)
    {
        channelStereoProvider = std::move(provider);
    }

    void setNumChannels(int num)
    {
        channelIds.clear();  // dense 1..N mode
        numChannels = juce::jmax(1, num);
        if (currentChannel > numChannels)
            setSelectedChannel(numChannels);
    }

    /** Restrict the selector to an explicit id list (stable channel numbers
        in display order; gaps possible). Tiles are labeled by id, selection
        snaps to the nearest live id. An empty list restores dense 1..N. */
    void setChannelIds(std::vector<int> ids)
    {
        channelIds = std::move(ids);
        if (channelIds.empty())
            return;
        numChannels = static_cast<int>(channelIds.size());
        if (! isLiveId(currentChannel))
            setSelectedChannel(nearestLiveId(currentChannel));
        else
            updateButtonText();
    }

    void setSelectedChannel(int channel)
    {
        currentChannel = snapToValid(channel);
        updateButtonText();
        if (onChannelChanged)
            onChannelChanged(currentChannel);
    }

    /** Select channel programmatically without risk of overlay opening.
     *  Use this when selecting via keyboard shortcuts to prevent the
     *  Enter key from also triggering the button's onClick.
     */
    void setSelectedChannelProgrammatically(int channel)
    {
        currentChannel = snapToValid(channel);
        updateButtonText();
        // Remove focus from the internal button to prevent Enter key from triggering overlay
        selectorButton.setWantsKeyboardFocus(false);
        if (onChannelChanged)
            onChannelChanged(currentChannel);
        // Re-enable keyboard focus after a short delay
        selectorButton.setWantsKeyboardFocus(true);
    }

    int getSelectedChannel() const { return currentChannel; }

    /** Re-reads the colour providers for the CURRENT selection.

        Needed when the data behind the colour moves without the selection
        moving — an output's array reassigned from the Outputs tab, from OSC,
        by "apply to array" or by a project load. Selection changes need no
        call: they already funnel through updateButtonText().
    */
    void refreshChannelColour() { applyTint(); }

    /** Installs a per-channel colour editor. When set, the popup grows a round colour
        swatch in its title bar, left of the close button; pressing it calls this with the
        selected channel and the swatch itself, so the caller can anchor a picker to it.

        Leaving it null (Outputs, Reverb) means no button and no behaviour change at all.
    */
    void setColourEditor(std::function<void(int, juce::Component&)> editor)
    {
        colourEditor = std::move(editor);
    }

    /** Repaints the collapsed button AND, if the grid happens to be open, its tiles.

        A colour can now be changed from inside the grid, and tiles are coloured in the
        overlay's resized(), which does not run again by itself. */
    void refreshChannelColours()
    {
        applyTint();
        if (auto* o = openOverlay.getComponent())
            o->refreshTileColours();
    }

    int getNumChannels() const { return numChannels; }

    /** Neighbour of the current selection in display order, wrapping.
        With an id list this walks the LIST (numbers may have gaps). */
    int adjacentChannel(int delta) const
    {
        if (channelIds.empty())
        {
            if (numChannels <= 1)
                return currentChannel;
            return ((currentChannel - 1 + delta % numChannels + numChannels) % numChannels) + 1;
        }

        const int n = static_cast<int>(channelIds.size());
        int idx = 0;
        for (int i = 0; i < n; ++i)
            if (channelIds[static_cast<size_t>(i)] == currentChannel)
                { idx = i; break; }
        idx = ((idx + delta) % n + n) % n;
        return channelIds[static_cast<size_t>(idx)];
    }

    void resized() override
    {
        selectorButton.setBounds(getLocalBounds());
    }

    std::function<void(int)> onChannelChanged;

private:
    void updateButtonText()
    {
        selectorButton.setButtonText(labelPrefix + " " + juce::String(currentChannel) + juce::String::fromUTF8(" ▼"));
        applyTint();
    }

    /** Paints the collapsed button in the selected channel's own colour, so the
        colour identifying a channel on its tile and its map marker survives the
        tile grid closing.

        Uses the SAME providers the popup tiles use, so each tab's existing rule
        carries over untouched — and a selector that installs no colour provider
        (the Reverb tab) clears the overrides and renders exactly as before.
    */
    void applyTint()
    {
        if (! channelColorProvider)
        {
            selectorButton.removeColour(juce::TextButton::buttonColourId);
            selectorButton.removeColour(juce::TextButton::textColourOffId);
            selectorButton.removeColour(juce::TextButton::textColourOnId);
            return;
        }

        const auto bg = channelColorProvider(currentChannel);
        const auto fg = textColorProvider ? textColorProvider(currentChannel)
                                          : WfsColorUtilities::getContrastingTextColor(bg);

        selectorButton.setColour(juce::TextButton::buttonColourId, bg);
        selectorButton.setColour(juce::TextButton::textColourOffId, fg);
        selectorButton.setColour(juce::TextButton::textColourOnId, fg);
    }

    void showOverlay()
    {
        if (auto* parent = getTopLevelComponent())
        {
            // Use SafePointers to handle potential component invalidation
            // Both this and parent can become invalid during callbacks
            juce::Component::SafePointer<juce::Component> safeParent = parent;
            juce::Component::SafePointer<ChannelSelectorButton> safeThis = this;

            // Callback to remove both backdrop and overlay
            auto removeOverlayComponents = [safeParent, safeThis](int selected) {
                juce::MessageManager::callAsync([safeThis, safeParent, selected]() {
                    if (safeParent == nullptr)
                        return;

                    auto* parent = safeParent.getComponent();

                    // Remove backdrop and overlay (in reverse order of addition)
                    for (int i = parent->getNumChildComponents() - 1; i >= 0; --i)
                    {
                        auto* child = parent->getChildComponent(i);
                        if (dynamic_cast<ChannelSelectorOverlay*>(child) ||
                            dynamic_cast<ChannelSelectorBackdrop*>(child))
                        {
                            parent->removeChildComponent(child);
                            delete child;
                        }
                    }

                    // Update channel selection (may trigger callbacks)
                    if (safeThis != nullptr)
                        safeThis->setSelectedChannel(selected);

                    // Grab focus on the selector button itself
                    juce::MessageManager::callAsync([safeThis]() {
                        if (safeThis != nullptr)
                            safeThis->grabKeyboardFocus();
                    });
                });
            };

            // Create backdrop for click-outside-to-dismiss
            auto backdrop = std::make_unique<ChannelSelectorBackdrop>([removeOverlayComponents, currentCh = currentChannel]() {
                removeOverlayComponents(currentCh);  // Dismiss without changing selection
            });
            backdrop->setBounds(parent->getLocalBounds());
            parent->addAndMakeVisible(backdrop.release());

            // Create overlay popup
            auto overlay = std::make_unique<ChannelSelectorOverlay>(
                numChannels,
                currentChannel,
                removeOverlayComponents,
                channelColorProvider,
                channelNameProvider,
                textColorProvider,
                channelIds,
                channelStereoProvider,
                colourEditor
            );

            openOverlay = overlay.get();

            // Get required size for the popup
            auto requiredSize = overlay->getRequiredSize();

            // Get button position relative to parent (top-level component)
            auto buttonBoundsInParent = parent->getLocalArea(this, getLocalBounds());

            // Position popup below the button, left-aligned
            int popupX = buttonBoundsInParent.getX();
            int popupY = buttonBoundsInParent.getBottom() + 4;

            // Ensure popup stays within parent bounds
            auto parentBounds = parent->getLocalBounds();

            // Adjust X if popup would overflow right edge
            if (popupX + requiredSize.x > parentBounds.getRight())
                popupX = parentBounds.getRight() - requiredSize.x;

            // Adjust X if it went negative
            if (popupX < 0)
                popupX = 0;

            // If popup would overflow bottom, show it above the button instead
            if (popupY + requiredSize.y > parentBounds.getBottom())
                popupY = buttonBoundsInParent.getY() - requiredSize.y - 4;

            // Adjust Y if it went negative
            if (popupY < 0)
                popupY = 0;

            overlay->setBounds(popupX, popupY, requiredSize.x, requiredSize.y);
            parent->addAndMakeVisible(overlay.release());
        }
    }

    bool isLiveId(int id) const
    {
        if (channelIds.empty())
            return id >= 1 && id <= numChannels;
        return std::find(channelIds.begin(), channelIds.end(), id) != channelIds.end();
    }

    int nearestLiveId(int id) const
    {
        if (channelIds.empty())
            return juce::jlimit(1, numChannels, id);
        int best = channelIds.front();
        for (int candidate : channelIds)
            if (std::abs(candidate - id) < std::abs(best - id))
                best = candidate;
        return best;
    }

    int snapToValid(int channel) const
    {
        return channelIds.empty() ? juce::jlimit(1, numChannels, channel)
                                  : nearestLiveId(channel);
    }

    juce::String labelPrefix;
    juce::TextButton selectorButton;
    int numChannels = 64;
    int currentChannel = 1;
    std::vector<int> channelIds;   // explicit id list (stable numbers); empty = dense 1..N
    std::function<juce::Colour(int)> channelColorProvider;
    std::function<juce::String(int)> channelNameProvider;
    std::function<juce::Colour(int)> textColorProvider;
    std::function<bool(int)> channelStereoProvider;
    std::function<void(int, juce::Component&)> colourEditor;
    juce::Component::SafePointer<ChannelSelectorOverlay> openOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelSelectorButton)
};
