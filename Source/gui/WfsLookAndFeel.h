#pragma once

#include <JuceHeader.h>
#include "ColorScheme.h"

/**
 * WfsLookAndFeel - Custom LookAndFeel for WFS-DIY
 *
 * Centralizes widget theming by pulling colors from ColorScheme.
 * Supports future multilingual font handling.
 */
class WfsLookAndFeel : public juce::LookAndFeel_V4,
                       public ColorScheme::Manager::Listener
{
public:
    WfsLookAndFeel()
    {
        ColorScheme::Manager::getInstance().addListener(this);
        updateFromColorScheme();
    }

    ~WfsLookAndFeel() override
    {
        ColorScheme::Manager::getInstance().removeListener(this);
    }

    /** Called when the color scheme changes */
    void colorSchemeChanged() override
    {
        updateFromColorScheme();
    }

    /** Update all widget colors from the current ColorScheme */
    void updateFromColorScheme()
    {
        const auto& colors = ColorScheme::get();

        //======================================================================
        // TextEditor
        setColour(juce::TextEditor::backgroundColourId, colors.surfaceCard);
        setColour(juce::TextEditor::textColourId, colors.textPrimary);
        setColour(juce::TextEditor::highlightColourId, colors.accentBlue);
        setColour(juce::TextEditor::highlightedTextColourId, colors.textPrimary);
        setColour(juce::TextEditor::outlineColourId, colors.buttonBorder);
        setColour(juce::TextEditor::focusedOutlineColourId, colors.accentBlue);

        //======================================================================
        // ComboBox
        setColour(juce::ComboBox::backgroundColourId, colors.surfaceCard);
        setColour(juce::ComboBox::textColourId, colors.textPrimary);
        setColour(juce::ComboBox::outlineColourId, colors.buttonBorder);
        setColour(juce::ComboBox::arrowColourId, colors.textSecondary);
        setColour(juce::ComboBox::focusedOutlineColourId, colors.accentBlue);

        //======================================================================
        // PopupMenu (used by ComboBox dropdowns)
        setColour(juce::PopupMenu::backgroundColourId, colors.surfaceCard);
        setColour(juce::PopupMenu::textColourId, colors.textPrimary);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, colors.listSelection);
        setColour(juce::PopupMenu::highlightedTextColourId, colors.textPrimary);
        setColour(juce::PopupMenu::headerTextColourId, colors.textSecondary);

        //======================================================================
        // TextButton
        setColour(juce::TextButton::buttonColourId, colors.buttonNormal);
        setColour(juce::TextButton::buttonOnColourId, colors.accentBlue);
        setColour(juce::TextButton::textColourOffId, colors.textPrimary);
        setColour(juce::TextButton::textColourOnId, colors.textPrimary);

        //======================================================================
        // ToggleButton
        setColour(juce::ToggleButton::textColourId, colors.textPrimary);
        setColour(juce::ToggleButton::tickColourId, colors.textPrimary);
        setColour(juce::ToggleButton::tickDisabledColourId, colors.textDisabled);

        //======================================================================
        // Label
        setColour(juce::Label::textColourId, colors.textPrimary);
        setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);

        //======================================================================
        // Slider
        setColour(juce::Slider::backgroundColourId, colors.sliderTrackBg);
        setColour(juce::Slider::trackColourId, colors.accentBlue);
        setColour(juce::Slider::thumbColourId, colors.sliderThumb);
        setColour(juce::Slider::textBoxTextColourId, colors.textPrimary);
        setColour(juce::Slider::textBoxBackgroundColourId, colors.surfaceCard);
        setColour(juce::Slider::textBoxOutlineColourId, colors.buttonBorder);
        setColour(juce::Slider::textBoxHighlightColourId, colors.accentBlue);

        //======================================================================
        // ListBox
        setColour(juce::ListBox::backgroundColourId, colors.listBackground);
        setColour(juce::ListBox::outlineColourId, colors.buttonBorder);
        setColour(juce::ListBox::textColourId, colors.textPrimary);

        //======================================================================
        // ScrollBar
        setColour(juce::ScrollBar::backgroundColourId, colors.backgroundAlt);
        setColour(juce::ScrollBar::thumbColourId, colors.sliderThumb);
        setColour(juce::ScrollBar::trackColourId, colors.backgroundAlt);

        //======================================================================
        // GroupComponent
        setColour(juce::GroupComponent::outlineColourId, colors.chromeDivider);
        setColour(juce::GroupComponent::textColourId, colors.textPrimary);

        //======================================================================
        // TabbedComponent / TabbedButtonBar
        setColour(juce::TabbedComponent::backgroundColourId, colors.tabBackground);
        setColour(juce::TabbedComponent::outlineColourId, colors.chromeDivider);
        setColour(juce::TabbedButtonBar::tabOutlineColourId, colors.chromeDivider);
        setColour(juce::TabbedButtonBar::tabTextColourId, colors.textPrimary);
        setColour(juce::TabbedButtonBar::frontOutlineColourId, colors.tabSelected);
        setColour(juce::TabbedButtonBar::frontTextColourId, colors.textPrimary);

        //======================================================================
        // AlertWindow
        setColour(juce::AlertWindow::backgroundColourId, colors.surfaceCard);
        setColour(juce::AlertWindow::textColourId, colors.textPrimary);
        setColour(juce::AlertWindow::outlineColourId, colors.chromeDivider);

        //======================================================================
        // ProgressBar
        setColour(juce::ProgressBar::backgroundColourId, colors.backgroundAlt);
        setColour(juce::ProgressBar::foregroundColourId, colors.accentBlue);

        //======================================================================
        // TreeView
        setColour(juce::TreeView::backgroundColourId, colors.listBackground);
        setColour(juce::TreeView::linesColourId, colors.chromeDivider);
        setColour(juce::TreeView::selectedItemBackgroundColourId, colors.listSelection);

        //======================================================================
        // TableHeaderComponent
        setColour(juce::TableHeaderComponent::backgroundColourId, colors.chromeBackground);
        setColour(juce::TableHeaderComponent::textColourId, colors.textPrimary);
        setColour(juce::TableHeaderComponent::outlineColourId, colors.chromeDivider);
        setColour(juce::TableHeaderComponent::highlightColourId, colors.listSelection);

        //======================================================================
        // Toolbar
        setColour(juce::Toolbar::backgroundColourId, colors.chromeBackground);
        setColour(juce::Toolbar::buttonMouseOverBackgroundColourId, colors.buttonHover);
        setColour(juce::Toolbar::buttonMouseDownBackgroundColourId, colors.buttonPressed);

        //======================================================================
        // Tooltip
        setColour(juce::TooltipWindow::backgroundColourId, colors.surfaceCard);
        setColour(juce::TooltipWindow::textColourId, colors.textPrimary);
        setColour(juce::TooltipWindow::outlineColourId, colors.chromeDivider);

        //======================================================================
        // ResizableWindow / DocumentWindow
        setColour(juce::ResizableWindow::backgroundColourId, colors.background);
        setColour(juce::DocumentWindow::textColourId, colors.textPrimary);

        //======================================================================
        // DirectoryContentsDisplayComponent (file browser)
        setColour(juce::DirectoryContentsDisplayComponent::textColourId, colors.textPrimary);
        setColour(juce::DirectoryContentsDisplayComponent::highlightColourId, colors.listSelection);
        setColour(juce::DirectoryContentsDisplayComponent::highlightedTextColourId, colors.textPrimary);

        //======================================================================
        // FileBrowserComponent
        setColour(juce::FileBrowserComponent::currentPathBoxBackgroundColourId, colors.surfaceCard);
        setColour(juce::FileBrowserComponent::currentPathBoxTextColourId, colors.textPrimary);
        setColour(juce::FileBrowserComponent::currentPathBoxArrowColourId, colors.textSecondary);
        setColour(juce::FileBrowserComponent::filenameBoxBackgroundColourId, colors.surfaceCard);
        setColour(juce::FileBrowserComponent::filenameBoxTextColourId, colors.textPrimary);

        //======================================================================
        // CaretComponent
        setColour(juce::CaretComponent::caretColourId, colors.textPrimary);

        //======================================================================
        // HyperlinkButton
        setColour(juce::HyperlinkButton::textColourId, colors.accentBlue);

        //======================================================================
        // PropertyComponent
        setColour(juce::PropertyComponent::backgroundColourId, colors.backgroundAlt);
        setColour(juce::PropertyComponent::labelTextColourId, colors.textPrimary);

        //======================================================================
        // BooleanPropertyComponent
        setColour(juce::BooleanPropertyComponent::backgroundColourId, colors.backgroundAlt);
        setColour(juce::BooleanPropertyComponent::outlineColourId, colors.buttonBorder);

        //======================================================================
        // TextPropertyComponent
        setColour(juce::TextPropertyComponent::backgroundColourId, colors.surfaceCard);
        setColour(juce::TextPropertyComponent::textColourId, colors.textPrimary);
        setColour(juce::TextPropertyComponent::outlineColourId, colors.buttonBorder);

        //======================================================================
        // KeyMappingEditorComponent
        setColour(juce::KeyMappingEditorComponent::backgroundColourId, colors.background);
        setColour(juce::KeyMappingEditorComponent::textColourId, colors.textPrimary);

        //======================================================================
        // CodeEditorComponent
        setColour(juce::CodeEditorComponent::backgroundColourId, colors.background);
        setColour(juce::CodeEditorComponent::defaultTextColourId, colors.textPrimary);
        setColour(juce::CodeEditorComponent::lineNumberBackgroundId, colors.backgroundAlt);
        setColour(juce::CodeEditorComponent::lineNumberTextId, colors.textSecondary);
        setColour(juce::CodeEditorComponent::highlightColourId, colors.listSelection);
    }

    //==========================================================================
    // Font methods - override these for multilingual support in the future

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        return juce::Font(juce::FontOptions(juce::jmin(15.0f * uiScale, (float)buttonHeight * 0.50f)));
    }

    juce::Font getLabelFont(juce::Label& label) override
    {
        // ComboBox internal labels already have a scaled font set by getComboBoxFont
        // via positionComboBoxText — don't double-scale them
        if (dynamic_cast<juce::ComboBox*>(label.getParentComponent()) != nullptr)
            return label.getFont();

        auto f = label.getFont();
        return f.withHeight(juce::jmax(10.0f, f.getHeight() * uiScale));
    }

    juce::Font getComboBoxFont(juce::ComboBox& box) override
    {
        return juce::Font(juce::FontOptions(juce::jmin(15.0f * uiScale, (float)box.getHeight() * 0.55f)));
    }

    juce::Font getPopupMenuFont() override
    {
        return juce::Font(juce::FontOptions(juce::jmax(10.0f, 14.0f * uiScale)));
    }

    juce::Font getAlertWindowMessageFont() override
    {
        return juce::Font(juce::FontOptions(juce::jmax(10.0f, 14.0f * uiScale)));
    }

    juce::Font getAlertWindowTitleFont() override
    {
        return juce::Font(juce::FontOptions(juce::jmax(12.0f, 17.0f * uiScale)).withStyle("Bold"));
    }

    //==========================================================================
    // Tooltip drawing - scale font and bounds for high-DPI

    void drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height) override
    {
        auto bounds = juce::Rectangle<int>(width, height);
        g.setColour(findColour(juce::TooltipWindow::backgroundColourId));
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
        g.setColour(findColour(juce::TooltipWindow::outlineColourId));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 4.0f, 1.0f);
        g.setColour(findColour(juce::TooltipWindow::textColourId));
        g.setFont(juce::FontOptions(juce::jmax(10.0f, 13.0f * uiScale)));
        g.drawFittedText(text, bounds.reduced(static_cast<int>(4.0f * uiScale)),
                         juce::Justification::centred, 4);
    }

    juce::Rectangle<int> getTooltipBounds(const juce::String& tipText,
                                           juce::Point<int> screenPos,
                                           juce::Rectangle<int> parentArea) override
    {
        auto font = juce::Font(juce::FontOptions(juce::jmax(10.0f, 13.0f * uiScale)));
        const int maxWidth = static_cast<int>(400.0f * uiScale);
        const int pad = static_cast<int>(8.0f * uiScale);

        juce::AttributedString s;
        s.setJustification(juce::Justification::centredLeft);
        s.append(tipText, font, findColour(juce::TooltipWindow::textColourId));

        juce::TextLayout tl;
        tl.createLayout(s, static_cast<float>(maxWidth));

        auto w = static_cast<int>(tl.getWidth()) + pad * 2;
        auto h = static_cast<int>(tl.getHeight()) + pad;

        return juce::Rectangle<int>(screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
                                     screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6) : screenPos.y + 6,
                                     w, h)
                .constrainedWithin(parentArea);
    }

    //==========================================================================
    // Toggle button drawing - scale tick box size for high-DPI

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto fontSize = juce::jmin(15.0f * uiScale, (float)button.getHeight() * 0.75f);
        auto tickWidth = fontSize * 1.1f;

        drawTickBox(g, button, 4.0f * uiScale,
                    ((float)button.getHeight() - tickWidth) * 0.5f,
                    tickWidth, tickWidth,
                    button.getToggleState(),
                    button.isEnabled(),
                    shouldDrawButtonAsHighlighted,
                    shouldDrawButtonAsDown);

        g.setColour(button.findColour(juce::ToggleButton::textColourId));
        g.setFont(juce::FontOptions(fontSize));

        if (!button.isEnabled())
            g.setOpacity(0.5f);

        g.drawFittedText(button.getButtonText(),
                         button.getLocalBounds()
                             .withTrimmedLeft(juce::roundToInt(tickWidth) + static_cast<int>(10.0f * uiScale))
                             .withTrimmedRight(2),
                         juce::Justification::centredLeft, 10);
    }

    //==========================================================================
    // Tab button drawing - override for proper contrast between selected/unselected

    void drawTabButton(juce::TabBarButton& button, juce::Graphics& g,
                       bool isMouseOver, bool isMouseDown) override
    {
        const auto& colors = ColorScheme::get();
        auto area = button.getActiveArea();
        bool isFrontTab = button.isFrontTab();

        // Background color based on state
        juce::Colour bgColor;
        if (isFrontTab)
            bgColor = colors.tabButtonSelected;
        else if (isMouseDown)
            bgColor = colors.buttonPressed;
        else if (isMouseOver)
            bgColor = colors.buttonHover;
        else
            bgColor = colors.tabButtonNormal;

        g.setColour(bgColor);
        g.fillRect(area);

        // Draw bottom border for selected tab (accent color indicator)
        if (isFrontTab)
        {
            int borderH = juce::jmax(2, static_cast<int>(3.0f * uiScale));
            g.setColour(colors.tabSelected);
            g.fillRect(area.getX(), area.getBottom() - borderH, area.getWidth(), borderH);
        }

        // Text color based on state
        juce::Colour textColor = isFrontTab ? colors.tabTextSelected : colors.tabTextNormal;
        g.setColour(textColor);

        // Draw the tab text with larger bold font
        auto font = juce::Font(juce::FontOptions(juce::jmax(10.0f, 15.0f * uiScale)).withStyle("Bold"));
        g.setFont(font);
        g.drawText(button.getButtonText(), area, juce::Justification::centred, true);
    }

    int getTabButtonBestWidth(juce::TabBarButton& /*button*/, int /*tabDepth*/) override
    {
        // Return fixed width so all tabs are equal size
        // Width calculated to fit "Live Source & Hackoustics" with generous padding
        return juce::jmax(140, static_cast<int>(220.0f * uiScale));
    }

    /** Global UI scale factor, set by MainComponent in its resized().
     *  1.0 = 1080p reference height. Used for font/tab scaling.
     */
    static inline float uiScale = 1.0f;

    /** Scale TextEditor fonts for all direct TextEditor children of a component.
     *  Call in each tab's resized() to keep number boxes proportional.
     */
    static void scaleTextEditorFonts(juce::Component& parent, float scale)
    {
        auto font = juce::Font(juce::FontOptions(juce::jmax(10.0f, 14.0f * scale)));
        for (int i = 0; i < parent.getNumChildComponents(); ++i)
            if (auto* te = dynamic_cast<juce::TextEditor*>(parent.getChildComponent(i)))
                te->applyFontToAllText(font, true);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WfsLookAndFeel)
};
