#include "PluginLookAndFeel.h"

namespace wfs::plugin
{
    PluginLookAndFeel::PluginLookAndFeel()
    {
        setupColours();
    }

    void PluginLookAndFeel::setupColours()
    {
        using juce::Colour;

        const auto bg            = Colour (DarkPalette::background);
        const auto bgAlt         = Colour (DarkPalette::backgroundAlt);
        const auto surface       = Colour (DarkPalette::surfaceCard);
        const auto divider       = Colour (DarkPalette::chromeDivider);
        const auto btnNormal     = Colour (DarkPalette::buttonNormal);
        const auto btnOn         = Colour (DarkPalette::buttonPressed);
        const auto btnBorder     = Colour (DarkPalette::buttonBorder);
        const auto text          = Colour (DarkPalette::textPrimary);
        const auto textDim       = Colour (DarkPalette::textSecondary);
        const auto accent        = Colour (DarkPalette::accentBlueBright);
        const auto accentDim     = Colour (DarkPalette::accentBlue);
        const auto trackBg       = Colour (DarkPalette::sliderTrackBg);
        const auto thumb         = Colour (DarkPalette::sliderThumb);

        setColour (juce::ResizableWindow::backgroundColourId,        bg);
        setColour (juce::DocumentWindow::backgroundColourId,         bg);
        setColour (juce::AlertWindow::backgroundColourId,            surface);
        setColour (juce::AlertWindow::textColourId,                  text);
        setColour (juce::AlertWindow::outlineColourId,               divider);

        setColour (juce::Label::textColourId,                        text);
        setColour (juce::Label::backgroundColourId,                  juce::Colours::transparentBlack);
        setColour (juce::Label::outlineColourId,                     juce::Colours::transparentBlack);

        setColour (juce::Slider::backgroundColourId,                 trackBg);
        setColour (juce::Slider::trackColourId,                      accent);
        setColour (juce::Slider::thumbColourId,                      thumb);
        setColour (juce::Slider::rotarySliderFillColourId,           accent);
        setColour (juce::Slider::rotarySliderOutlineColourId,        divider);
        setColour (juce::Slider::textBoxTextColourId,                text);
        setColour (juce::Slider::textBoxBackgroundColourId,          surface);
        setColour (juce::Slider::textBoxHighlightColourId,           accentDim);
        setColour (juce::Slider::textBoxOutlineColourId,             divider);

        setColour (juce::TextButton::buttonColourId,                 btnNormal);
        setColour (juce::TextButton::buttonOnColourId,               btnOn);
        setColour (juce::TextButton::textColourOnId,                 text);
        setColour (juce::TextButton::textColourOffId,                text);
        setColour (juce::ComboBox::buttonColourId,                   btnBorder);

        setColour (juce::ToggleButton::textColourId,                 text);
        setColour (juce::ToggleButton::tickColourId,                 accent);
        setColour (juce::ToggleButton::tickDisabledColourId,         divider);

        setColour (juce::TextEditor::backgroundColourId,             surface);
        setColour (juce::TextEditor::textColourId,                   text);
        setColour (juce::TextEditor::highlightColourId,              accentDim);
        setColour (juce::TextEditor::highlightedTextColourId,        text);
        setColour (juce::TextEditor::outlineColourId,                divider);
        setColour (juce::TextEditor::focusedOutlineColourId,         accent);
        setColour (juce::CaretComponent::caretColourId,              accent);

        setColour (juce::ComboBox::backgroundColourId,               surface);
        setColour (juce::ComboBox::textColourId,                     text);
        setColour (juce::ComboBox::outlineColourId,                  divider);
        setColour (juce::ComboBox::arrowColourId,                    textDim);
        setColour (juce::ComboBox::focusedOutlineColourId,           accent);

        setColour (juce::PopupMenu::backgroundColourId,              surface);
        setColour (juce::PopupMenu::textColourId,                    text);
        setColour (juce::PopupMenu::headerTextColourId,              textDim);
        setColour (juce::PopupMenu::highlightedBackgroundColourId,   accentDim);
        setColour (juce::PopupMenu::highlightedTextColourId,         text);

        setColour (juce::ScrollBar::backgroundColourId,              bg);
        setColour (juce::ScrollBar::thumbColourId,                   divider);
        setColour (juce::ScrollBar::trackColourId,                   bgAlt);

        setColour (juce::TabbedComponent::backgroundColourId,        bg);
        setColour (juce::TabbedComponent::outlineColourId,           divider);
        setColour (juce::TabbedButtonBar::tabOutlineColourId,        divider);
        setColour (juce::TabbedButtonBar::tabTextColourId,           Colour (DarkPalette::tabTextNormal));
        setColour (juce::TabbedButtonBar::frontOutlineColourId,      accent);
        setColour (juce::TabbedButtonBar::frontTextColourId,         Colour (DarkPalette::tabTextSelected));
    }

    juce::Font PluginLookAndFeel::getLabelFont (juce::Label& l)
    {
        return l.getFont();
    }

    juce::Font PluginLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
    {
        return juce::FontOptions (juce::jmin (15.0f, buttonHeight * 0.55f));
    }

    juce::Font PluginLookAndFeel::getComboBoxFont (juce::ComboBox&)
    {
        return juce::FontOptions (15.0f);
    }

    juce::Font PluginLookAndFeel::getPopupMenuFont()
    {
        return juce::FontOptions (14.0f);
    }
}
