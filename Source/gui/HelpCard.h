#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ColorScheme.h"
#include "WfsLookAndFeel.h"
#include "../Localization/LocalizationManager.h"

//==============================================================================
/**
 * Reusable help card that displays a title, body text, and optional illustration.
 * Appears as a floating card with drop shadow, toggled by a HelpCardButton.
 */
class HelpCard : public juce::Component,
                 private ColorScheme::Manager::Listener
{
public:
    HelpCard()
    {
        setOpaque(false);
        setAlwaysOnTop(true);

        addAndMakeVisible(titleLabel);
        titleLabel.setJustificationType(juce::Justification::centredLeft);

        addAndMakeVisible(bodyLabel);
        bodyLabel.setJustificationType(juce::Justification::topLeft);

        updateColors();
        ColorScheme::Manager::getInstance().addListener(this);
    }

    ~HelpCard() override
    {
        ColorScheme::Manager::getInstance().removeListener(this);
        if (auto* p = getParentComponent())
            p->removeMouseListener(this);
    }

    void setContent(const juce::String& newTitle, const juce::String& newBody)
    {
        titleLabel.setText(newTitle, juce::dontSendNotification);
        bodyLabel.setText(newBody, juce::dontSendNotification);
    }

    void setIllustration(const juce::Image& img)
    {
        illustration = img;
        repaint();
    }

    /** Calculate the ideal height for a given width, based on text content. */
    int getIdealHeight(int width) const
    {
        float scale = WfsLookAndFeel::uiScale;
        int padding = 40; // 20px each side
        int titleH = (int)(28 * scale);
        int textWidth = width - padding;

        // Illustration height
        int illustrationH = 0;
        if (illustration.isValid())
        {
            float aspect = (float)illustration.getWidth() / (float)illustration.getHeight();
            illustrationH = (int)((float)textWidth / aspect) + 10;
        }

        // Estimate body height from text wrapping
        auto bodyText = bodyLabel.getText();
        auto bodyFont = juce::Font(juce::FontOptions().withHeight(juce::jmax(13.0f, 16.0f * scale)));
        float lineH = bodyFont.getHeight() * 1.4f;

        juce::GlyphArrangement glyphs;
        glyphs.addJustifiedText(bodyFont, bodyText, 0.0f, 0.0f, (float)textWidth, juce::Justification::left);
        int numLines = glyphs.getNumGlyphs() > 0
            ? (int)((glyphs.getBoundingBox(glyphs.getNumGlyphs() - 1, 1, true).getBottom()) / lineH) + 1
            : 1;
        int bodyHeight = juce::jmax((int)(30 * scale), (int)(numLines * lineH + lineH));

        return padding + titleH + 8 + illustrationH + bodyHeight + padding / 2;
    }

    /** Show the card and register click-outside listener on parent. */
    void show()
    {
        setVisible(true);
        toFront(false);
        if (auto* p = getParentComponent())
            p->addMouseListener(this, true);
    }

    /** Hide the card and unregister click-outside listener. */
    void hide()
    {
        if (!isVisible()) return;
        setVisible(false);
        if (auto* p = getParentComponent())
            p->removeMouseListener(this);
        if (onDismissed) onDismissed();
    }

    /** The button that toggles this card, set so we can ignore clicks on it. */
    void setToggleButton(juce::Component* btn) { toggleButton = btn; }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Shadow
        juce::DropShadow shadow(juce::Colours::black.withAlpha(0.35f), 10, {0, 3});
        shadow.drawForRectangle(g, getLocalBounds());

        // Background
        g.setColour(ColorScheme::get().surfaceCard);
        g.fillRoundedRectangle(bounds, 8.0f);

        // Border
        g.setColour(ColorScheme::get().buttonBorder);
        g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

        // Illustration (if any)
        if (illustration.isValid())
        {
            auto area = getLocalBounds().reduced(20);
            area.removeFromTop((int)(28 * WfsLookAndFeel::uiScale) + 8); // skip title
            float aspect = (float)illustration.getWidth() / (float)illustration.getHeight();
            int imgH = (int)((float)area.getWidth() / aspect);
            g.drawImage(illustration, area.removeFromTop(imgH).toFloat(),
                        juce::RectanglePlacement::centred);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(20);
        float scale = WfsLookAndFeel::uiScale;

        int titleH = (int)(28 * scale);
        titleLabel.setBounds(area.removeFromTop(titleH));
        area.removeFromTop(8);

        // Skip illustration space
        if (illustration.isValid())
        {
            float aspect = (float)illustration.getWidth() / (float)illustration.getHeight();
            int imgH = (int)((float)area.getWidth() / aspect);
            area.removeFromTop(imgH + 10);
        }

        // Body takes remaining space
        bodyLabel.setBounds(area);
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        // Dismiss when clicking outside the card (but not on the toggle button)
        if (!isVisible()) return;

        auto clickPos = e.getEventRelativeTo(this).getPosition();
        if (!getLocalBounds().contains(clickPos))
        {
            // Check if the click is on the toggle button — if so, let the button handle it
            if (toggleButton != nullptr)
            {
                auto btnClick = e.getEventRelativeTo(toggleButton).getPosition();
                if (toggleButton->getLocalBounds().contains(btnClick))
                    return;
            }
            hide();
        }
    }

    std::function<void()> onDismissed;

private:
    juce::Label titleLabel;
    juce::Label bodyLabel;
    juce::Image illustration;
    juce::Component* toggleButton = nullptr;

    void colorSchemeChanged() override
    {
        updateColors();
        repaint();
    }

    void updateColors()
    {
        auto& palette = ColorScheme::get();
        float scale = WfsLookAndFeel::uiScale;

        titleLabel.setColour(juce::Label::textColourId, palette.textPrimary);
        titleLabel.setFont(juce::FontOptions().withHeight(juce::jmax(14.0f, 18.0f * scale)).withStyle("Bold"));

        bodyLabel.setColour(juce::Label::textColourId, palette.textSecondary);
        bodyLabel.setFont(juce::FontOptions().withHeight(juce::jmax(13.0f, 16.0f * scale)));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelpCard)
};

//==============================================================================
/**
 * Scrollable help card for long content. Title stays fixed at top,
 * body text scrolls in a viewport. Same toggle/dismiss pattern as HelpCard.
 */
class ScrollableHelpCard : public juce::Component,
                            private ColorScheme::Manager::Listener
{
public:
    ScrollableHelpCard()
    {
        setOpaque(false);
        setAlwaysOnTop(true);

        addAndMakeVisible(titleLabel);
        titleLabel.setJustificationType(juce::Justification::centredLeft);

        viewport.setViewedComponent(&bodyContent, false);
        viewport.setScrollBarsShown(true, false);
        addAndMakeVisible(viewport);

        updateColors();
        ColorScheme::Manager::getInstance().addListener(this);
    }

    ~ScrollableHelpCard() override
    {
        ColorScheme::Manager::getInstance().removeListener(this);
        if (auto* p = getParentComponent())
            p->removeMouseListener(this);
    }

    void setContent(const juce::String& newTitle, const juce::String& newBody)
    {
        title = newTitle;
        body = newBody;
        titleLabel.setText(newTitle, juce::dontSendNotification);
        bodyContent.bodyText = newBody;
    }

    void setToggleButton(juce::Component* btn) { toggleButton = btn; }

    void show()
    {
        setVisible(true);
        toFront(false);
        if (auto* p = getParentComponent())
            p->addMouseListener(this, true);
    }

    void hide()
    {
        if (!isVisible()) return;
        setVisible(false);
        if (auto* p = getParentComponent())
            p->removeMouseListener(this);
        if (onDismissed) onDismissed();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        juce::DropShadow shadow(juce::Colours::black.withAlpha(0.35f), 10, {0, 3});
        shadow.drawForRectangle(g, getLocalBounds());

        g.setColour(ColorScheme::get().surfaceCard);
        g.fillRoundedRectangle(bounds, 8.0f);

        g.setColour(ColorScheme::get().buttonBorder);
        g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(20);
        float scale = WfsLookAndFeel::uiScale;

        int titleH = (int)(28 * scale);
        titleLabel.setBounds(area.removeFromTop(titleH));
        area.removeFromTop(8);

        viewport.setBounds(area);

        // Size body content to fit all text
        int contentW = area.getWidth() - 14; // scrollbar width
        auto bodyFont = juce::Font(juce::FontOptions().withHeight(juce::jmax(13.0f, 16.0f * scale)));
        float lineH = bodyFont.getHeight() * 1.4f;

        juce::GlyphArrangement glyphs;
        glyphs.addJustifiedText(bodyFont, body, 0.0f, 0.0f, (float)contentW, juce::Justification::left);
        int numLines = glyphs.getNumGlyphs() > 0
            ? (int)((glyphs.getBoundingBox(glyphs.getNumGlyphs() - 1, 1, true).getBottom()) / lineH) + 1
            : 1;
        int bodyHeight = (int)(numLines * lineH + lineH);

        bodyContent.setSize(contentW, bodyHeight);
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (!isVisible()) return;
        auto clickPos = e.getEventRelativeTo(this).getPosition();
        if (!getLocalBounds().contains(clickPos))
        {
            if (toggleButton != nullptr)
            {
                auto btnClick = e.getEventRelativeTo(toggleButton).getPosition();
                if (toggleButton->getLocalBounds().contains(btnClick))
                    return;
            }
            hide();
        }
    }

    std::function<void()> onDismissed;

private:
    juce::String title, body;
    juce::Label titleLabel;
    juce::Component* toggleButton = nullptr;

    class BodyComponent : public juce::Component
    {
    public:
        void paint(juce::Graphics& g) override
        {
            float scale = WfsLookAndFeel::uiScale;
            auto font = juce::Font(juce::FontOptions().withHeight(juce::jmax(13.0f, 16.0f * scale)));
            g.setColour(ColorScheme::get().textSecondary);
            g.setFont(font);
            g.drawFittedText(bodyText, getLocalBounds(), juce::Justification::topLeft, 1000);
        }
        juce::String bodyText;
    };

    BodyComponent bodyContent;
    juce::Viewport viewport;

    void colorSchemeChanged() override
    {
        auto& palette = ColorScheme::get();
        float scale = WfsLookAndFeel::uiScale;
        titleLabel.setColour(juce::Label::textColourId, palette.textPrimary);
        titleLabel.setFont(juce::FontOptions().withHeight(juce::jmax(14.0f, 20.0f * scale)).withStyle("Bold"));
        repaint();
        bodyContent.repaint();
    }

    void updateColors() { colorSchemeChanged(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScrollableHelpCard)
};

//==============================================================================
/**
 * Small round "?" button that toggles a HelpCard's visibility.
 * Subdued appearance that brightens on hover and when active.
 */
class HelpCardButton : public juce::Component,
                       private ColorScheme::Manager::Listener
{
public:
    HelpCardButton()
    {
        setOpaque(false);
        setWantsKeyboardFocus(true);
        setMouseClickGrabsKeyboardFocus(true);
        ColorScheme::Manager::getInstance().addListener(this);
    }

    ~HelpCardButton() override
    {
        ColorScheme::Manager::getInstance().removeListener(this);
    }

    void setCard(HelpCard* card)
    {
        associatedCard = card;
        scrollableCard = nullptr;
        if (card != nullptr)
        {
            card->setToggleButton(this);
            card->onDismissed = [this]() { isActive = false; repaint(); };
        }
    }

    void setCard(ScrollableHelpCard* card)
    {
        scrollableCard = card;
        associatedCard = nullptr;
        if (card != nullptr)
        {
            card->setToggleButton(this);
            card->onDismissed = [this]() { isActive = false; repaint(); };
        }
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        auto& palette = ColorScheme::get();

        // Circle fill — solid contrasting background
        if (isActive)
            g.setColour(palette.accentBlue.withAlpha(0.3f));
        else if (isHovered)
            g.setColour(palette.surfaceCard.brighter(0.15f));
        else
            g.setColour(palette.surfaceCard);
        g.fillEllipse(bounds);

        // Circle border — visible even at idle
        auto borderColour = isActive ? palette.accentBlue : palette.textSecondary;
        g.setColour(borderColour.withAlpha(isActive ? 1.0f : (isHovered ? 0.8f : 0.6f)));
        g.drawEllipse(bounds, 1.5f);

        // "?" glyph — readable at idle
        float fontSize = bounds.getHeight() * 0.6f;
        g.setColour(borderColour.withAlpha(isActive ? 1.0f : (isHovered ? 0.9f : 0.75f)));
        g.setFont(juce::FontOptions().withHeight(fontSize).withStyle("Bold"));
        g.drawText("?", bounds.toNearestInt(), juce::Justification::centred);
    }

    void mouseEnter(const juce::MouseEvent&) override  { isHovered = true;  repaint(); }
    void mouseExit(const juce::MouseEvent&) override   { isHovered = false; repaint(); }

    void mouseDown(const juce::MouseEvent&) override
    {
        // Grab focus away from any text editor to prevent focus stealing
        grabKeyboardFocus();
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (!e.mouseWasClicked() || !getLocalBounds().contains(e.getPosition()))
            return;

        if (associatedCard == nullptr && scrollableCard == nullptr) return;

        isActive = !isActive;
        if (associatedCard != nullptr)
        {
            if (isActive) associatedCard->show(); else associatedCard->hide();
        }
        else if (scrollableCard != nullptr)
        {
            if (isActive) scrollableCard->show(); else scrollableCard->hide();
        }
        repaint();
    }

    /** Dismiss the card programmatically (e.g. when leaving a tab). */
    void dismiss()
    {
        if (isActive)
        {
            if (associatedCard != nullptr) associatedCard->hide();
            if (scrollableCard != nullptr) scrollableCard->hide();
            isActive = false;
            repaint();
        }
    }

    void visibilityChanged() override
    {
        // Auto-dismiss when the button becomes invisible (tab switch)
        if (!isVisible())
            dismiss();
    }

private:
    HelpCard* associatedCard = nullptr;
    ScrollableHelpCard* scrollableCard = nullptr;
    bool isActive = false;
    bool isHovered = false;

    void colorSchemeChanged() override { repaint(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelpCardButton)
};
