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

        // Optional code block + copy button — invisible until setCodeBlock()
        // is called with non-empty text.
        codeEditor.setMultiLine(true, true);
        codeEditor.setReadOnly(true);
        codeEditor.setScrollbarsShown(true);
        codeEditor.setCaretVisible(false);
        codeEditor.setVisible(false);
        addAndMakeVisible(codeEditor);

        copyButton.setVisible(false);
        copyButton.onClick = [this] { if (onCopyClicked) onCopyClicked(); };
        addAndMakeVisible(copyButton);

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

    /** Optional read-only code block + Copy button shown below the body.
        Pass an empty string to hide. The Copy button calls `onCopyClicked`
        — the host wires that callback to whatever clipboard write makes
        sense (the JSON snippet may differ from the displayed code if
        runtime substitution is needed). */
    void setCodeBlock(const juce::String& codeText,
                      const juce::String& copyButtonLabel = "Copy")
    {
        const bool hasCode = codeText.isNotEmpty();
        codeEditor.setText(codeText, juce::dontSendNotification);
        codeEditor.setVisible(hasCode);
        copyButton.setVisible(hasCode);
        copyButton.setButtonText(copyButtonLabel);
        codeBlockHeightCache = -1;  // force recomputation
        updateColors();
        resized();
    }

    /** Set a custom font scale multiplier (default 1.0) */
    void setFontScale(float scale) { fontScale = scale; updateColors(); }

    void setIllustration(const juce::Image& img)
    {
        illustration = img;
        repaint();
    }

    /** Calculate the ideal height for a given width, based on text content. */
    int getIdealHeight(int width) const
    {
        float scale = WfsLookAndFeel::uiScale;
        int padding = 28; // 14px each side
        int titleH = (int)(34 * scale * fontScale);
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
        auto bodyFont = juce::Font(juce::FontOptions().withHeight(juce::jmax(15.0f, 19.0f * scale * fontScale)));
        float lineH = bodyFont.getHeight() * 1.4f;

        juce::GlyphArrangement glyphs;
        glyphs.addJustifiedText(bodyFont, bodyText, 0.0f, 0.0f, (float)textWidth, juce::Justification::left);
        int numLines = glyphs.getNumGlyphs() > 0
            ? (int)((glyphs.getBoundingBox(glyphs.getNumGlyphs() - 1, 1, true).getBottom()) / lineH) + 1
            : 1;
        int bodyHeight = juce::jmax((int)(30 * scale), (int)(numLines * lineH + lineH));

        // Optional code block + copy button — auto-sized to the code's
        // line count, capped to a sensible maximum.
        int codeBlockH = 0;
        if (codeEditor.isVisible())
        {
            auto codeFont = juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
                                                         juce::jmax(12.0f, 13.0f * scale), juce::Font::plain));
            float codeLineH = codeFont.getHeight() * 1.25f;
            int codeLines = juce::jmax(3, codeEditor.getText().getCharPointer().lengthUpTo(20000) > 0
                                            ? juce::StringArray::fromLines(codeEditor.getText()).size()
                                            : 3);
            codeLines = juce::jmin(codeLines, 14);  // cap to keep the card sane
            int copyButtonH = (int)(28 * scale);
            codeBlockH = (int)(codeLines * codeLineH) + 12 + copyButtonH + 12;
        }

        return padding + titleH + 8 + illustrationH + bodyHeight + codeBlockH + padding / 2;
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
            auto area = getLocalBounds().reduced(14);
            area.removeFromTop((int)(28 * WfsLookAndFeel::uiScale) + 8); // skip title
            float aspect = (float)illustration.getWidth() / (float)illustration.getHeight();
            int imgH = (int)((float)area.getWidth() / aspect);
            g.drawImage(illustration, area.removeFromTop(imgH).toFloat(),
                        juce::RectanglePlacement::centred);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(14);
        float scale = WfsLookAndFeel::uiScale;

        int titleH = (int)(34 * scale);
        titleLabel.setBounds(area.removeFromTop(titleH));
        area.removeFromTop(8);

        // Skip illustration space
        if (illustration.isValid())
        {
            float aspect = (float)illustration.getWidth() / (float)illustration.getHeight();
            int imgH = (int)((float)area.getWidth() / aspect);
            area.removeFromTop(imgH + 10);
        }

        // Optional code block + copy button anchored to the bottom.
        if (codeEditor.isVisible())
        {
            int copyButtonH = (int)(28 * scale);
            int copyButtonW = (int)(110 * scale);

            auto codeBlockArea = area;
            // Reserve at least 3 monospace lines + button + spacing for the
            // code block; everything above is body.
            auto codeFont = juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
                                                         juce::jmax(12.0f, 13.0f * scale), juce::Font::plain));
            float codeLineH = codeFont.getHeight() * 1.25f;
            int codeLines = juce::jmin(14, juce::jmax(3, juce::StringArray::fromLines(codeEditor.getText()).size()));
            int codeAreaH = (int)(codeLines * codeLineH) + 12 + copyButtonH + 12;
            codeAreaH = juce::jmin(codeAreaH, area.getHeight() - 30);

            auto codeArea = area.removeFromBottom(codeAreaH);
            auto buttonRow = codeArea.removeFromBottom(copyButtonH);
            copyButton.setBounds(buttonRow.removeFromRight(copyButtonW));
            codeArea.removeFromBottom(8);
            codeEditor.setBounds(codeArea);
        }

        // Body takes the remaining space above the code block (if any).
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

    /** Fired when the optional code block's Copy button is clicked. The host
        wires this — the card is intentionally agnostic to clipboard format
        so callers can substitute live values (URL, port, etc.) at copy time. */
    std::function<void()> onCopyClicked;

private:
    float fontScale = 1.0f;
    juce::Label titleLabel;
    juce::Label bodyLabel;
    juce::TextEditor codeEditor;
    juce::TextButton copyButton { "Copy" };
    int codeBlockHeightCache = -1;
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
        titleLabel.setFont(juce::FontOptions().withHeight(juce::jmax(16.0f, 22.0f * scale * fontScale)).withStyle("Bold"));

        bodyLabel.setColour(juce::Label::textColourId, palette.textSecondary);
        bodyLabel.setFont(juce::FontOptions().withHeight(juce::jmax(15.0f, 19.0f * scale * fontScale)));

        // Code block — monospaced, dimmed background, primary text colour.
        codeEditor.setColour(juce::TextEditor::backgroundColourId, palette.background.darker(0.3f));
        codeEditor.setColour(juce::TextEditor::textColourId,       palette.textPrimary);
        codeEditor.setColour(juce::TextEditor::outlineColourId,    palette.buttonBorder);
        codeEditor.setColour(juce::TextEditor::focusedOutlineColourId, palette.buttonBorder);
        codeEditor.applyFontToAllText(juce::Font(juce::FontOptions(
            juce::Font::getDefaultMonospacedFontName(),
            juce::jmax(12.0f, 13.0f * scale), juce::Font::plain)));

        copyButton.setColour(juce::TextButton::buttonColourId, palette.buttonNormal);
        copyButton.setColour(juce::TextButton::textColourOffId, palette.textPrimary);
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
        // Simple mode: single text section
        bodyContent.sections.clear();
        bodyContent.sections.push_back({ newBody, nullptr, 0 });
    }

    /** Add a text section */
    void addTextSection(const juce::String& text)
    {
        bodyContent.sections.push_back({ text, nullptr, 0 });
    }

    /** Add an illustration section with a custom draw callback and fixed height */
    void addIllustration(int height, std::function<void(juce::Graphics&, juce::Rectangle<int>)> drawFunc)
    {
        bodyContent.sections.push_back({ {}, std::move(drawFunc), height });
    }

    /** Hide the scrollbar (for cards that fit without scrolling) */
    void setScrollBarVisible(bool visible)
    {
        viewport.setScrollBarsShown(visible, false);
    }

    /** Clear all sections (for building content incrementally) */
    void clearSections()
    {
        bodyContent.sections.clear();
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
        auto area = getLocalBounds().reduced(14);
        float scale = WfsLookAndFeel::uiScale;

        int titleH = (int)(34 * scale);
        titleLabel.setBounds(area.removeFromTop(titleH));
        area.removeFromTop(8);

        viewport.setBounds(area);

        // Size body content to fit all sections
        int contentW = area.getWidth() - 14; // scrollbar width
        int bodyHeight = bodyContent.sections.empty()
            ? bodyContent.calculateHeight(contentW) // fallback if no sections set
            : bodyContent.calculateHeight(contentW);
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

    /** A section is either text or an illustration callback */
    struct Section
    {
        juce::String text;                                           // non-empty for text sections
        std::function<void(juce::Graphics&, juce::Rectangle<int>)> drawIllustration; // for illustrations
        int illustrationHeight = 0;                                  // pixel height for illustration sections
    };

    class BodyComponent : public juce::Component
    {
    public:
        void paint(juce::Graphics& g) override
        {
            float scale = WfsLookAndFeel::uiScale;
            auto font = juce::Font(juce::FontOptions().withHeight(juce::jmax(15.0f, 19.0f * scale)));
            int w = getWidth();
            int y = 0;

            for (auto& section : sections)
            {
                if (section.drawIllustration)
                {
                    // Draw illustration
                    section.drawIllustration(g, { 0, y, w, section.illustrationHeight });
                    y += section.illustrationHeight;
                }
                else
                {
                    // Draw text
                    g.setColour(ColorScheme::get().textSecondary);
                    g.setFont(font);
                    float lineH = font.getHeight() * 1.4f;
                    juce::GlyphArrangement glyphs;
                    glyphs.addJustifiedText(font, section.text, 0.0f, (float)y + lineH,
                                            (float)w, juce::Justification::left);
                    glyphs.draw(g);
                    int numLines = glyphs.getNumGlyphs() > 0
                        ? (int)((glyphs.getBoundingBox(glyphs.getNumGlyphs() - 1, 1, true).getBottom() - (float)y) / lineH) + 1
                        : 1;
                    y += (int)(numLines * lineH + lineH * 0.5f);
                }
            }
        }

        int calculateHeight(int width) const
        {
            float scale = WfsLookAndFeel::uiScale;
            auto font = juce::Font(juce::FontOptions().withHeight(juce::jmax(15.0f, 19.0f * scale)));
            float lineH = font.getHeight() * 1.4f;
            int totalH = 0;

            for (auto& section : sections)
            {
                if (section.drawIllustration)
                {
                    totalH += section.illustrationHeight;
                }
                else
                {
                    // Match paint() logic: text starts at y + lineH baseline
                    juce::GlyphArrangement glyphs;
                    glyphs.addJustifiedText(font, section.text, 0.0f, lineH, (float)width, juce::Justification::left);
                    float textBottom = lineH; // minimum one line
                    if (glyphs.getNumGlyphs() > 0)
                        textBottom = glyphs.getBoundingBox(glyphs.getNumGlyphs() - 1, 1, true).getBottom();
                    totalH += (int)(textBottom + lineH);
                }
            }
            return totalH + (int)lineH;
        }

        std::vector<Section> sections;
        juce::String bodyText; // kept for simple text-only mode
    };

    BodyComponent bodyContent;
    juce::Viewport viewport;

    void colorSchemeChanged() override
    {
        auto& palette = ColorScheme::get();
        float scale = WfsLookAndFeel::uiScale;
        titleLabel.setColour(juce::Label::textColourId, palette.textPrimary);
        titleLabel.setFont(juce::FontOptions().withHeight(juce::jmax(16.0f, 24.0f * scale)).withStyle("Bold"));
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

    /** Activate the card programmatically (e.g. from H key cycling). */
    void activate()
    {
        if (!isActive)
        {
            isActive = true;
            if (associatedCard != nullptr) associatedCard->show();
            else if (scrollableCard != nullptr) scrollableCard->show();
            repaint();
        }
    }

    /** Check if the card is currently active/visible. */
    bool getIsActive() const { return isActive; }

    void visibilityChanged() override
    {
        // Auto-dismiss when the button becomes invisible (tab switch)
        if (!isVisible())
            dismiss();
    }

    void parentHierarchyChanged() override
    {
        // Also dismiss when parent hierarchy changes (main tab switch)
        if (!isShowing())
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

//==============================================================================
/**
 * Interface for components that provide help cards accessible via H key cycling.
 * Implement in tabs/windows to return the help buttons relevant to the current view.
 */
class HelpCardProvider
{
public:
    virtual ~HelpCardProvider() = default;
    virtual std::vector<HelpCardButton*> getVisibleHelpButtons() = 0;
};
