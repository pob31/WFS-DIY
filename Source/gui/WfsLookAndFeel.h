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
        return juce::Font(juce::FontOptions(juce::jmin(15.0f, (float)buttonHeight * 0.6f)));
    }

    juce::Font getLabelFont(juce::Label& label) override
    {
        return label.getFont();
    }

    juce::Font getComboBoxFont(juce::ComboBox& box) override
    {
        return juce::Font(juce::FontOptions(juce::jmin(15.0f, (float)box.getHeight() * 0.85f)));
    }

    juce::Font getPopupMenuFont() override
    {
        return juce::Font(juce::FontOptions(14.0f));
    }

    juce::Font getAlertWindowMessageFont() override
    {
        return juce::Font(juce::FontOptions(14.0f));
    }

    juce::Font getAlertWindowTitleFont() override
    {
        return juce::Font(juce::FontOptions(17.0f).withStyle("Bold"));
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
            g.setColour(colors.tabSelected);
            g.fillRect(area.getX(), area.getBottom() - 3, area.getWidth(), 3);
        }

        // Text color based on state
        juce::Colour textColor = isFrontTab ? colors.tabTextSelected : colors.tabTextNormal;
        g.setColour(textColor);

        // Draw the tab text with larger bold font
        auto font = juce::Font(juce::FontOptions(15.0f).withStyle("Bold"));
        g.setFont(font);
        g.drawText(button.getButtonText(), area, juce::Justification::centred, true);
    }

    int getTabButtonBestWidth(juce::TabBarButton& /*button*/, int /*tabDepth*/) override
    {
        // Return fixed width so all tabs are equal size
        // Width calculated to fit "System Configuration" with generous padding
        return 175;
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WfsLookAndFeel)
};
