#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace wfs::plugin
{
    // Mirrors `ColorScheme::DefaultPalette` from the main WFS-DIY app so the
    // plugin UI visually matches the default (dark grey) theme. If the main
    // app's palette changes, update these values in sync.
    struct DarkPalette
    {
        static constexpr juce::uint32 background        = 0xFF1E1E1E;
        static constexpr juce::uint32 backgroundAlt     = 0xFF252525;
        static constexpr juce::uint32 surfaceCard       = 0xFF2A2A2A;
        static constexpr juce::uint32 chromeBackground  = 0xFF606060;
        static constexpr juce::uint32 chromeSurface     = 0xFF252525;
        static constexpr juce::uint32 chromeDivider     = 0xFF404040;

        static constexpr juce::uint32 buttonNormal      = 0xFF2A2A2A;
        static constexpr juce::uint32 buttonHover       = 0xFF353535;
        static constexpr juce::uint32 buttonPressed     = 0xFF404040;
        static constexpr juce::uint32 buttonBorder      = 0xFF606060;

        static constexpr juce::uint32 textPrimary       = 0xFFFFFFFE;
        static constexpr juce::uint32 textSecondary     = 0xFFAAAAAA;
        static constexpr juce::uint32 textDisabled      = 0xFF808080;

        static constexpr juce::uint32 accentBlue        = 0xFF33668C;
        static constexpr juce::uint32 accentBlueBright  = 0xFF4A90D9;
        static constexpr juce::uint32 accentRed         = 0xFF8C3333;
        static constexpr juce::uint32 accentGreen       = 0xFF338C33;
        static constexpr juce::uint32 accentGreenDark   = 0xFF266626;

        static constexpr juce::uint32 sliderTrackBg     = 0xFF000000;
        static constexpr juce::uint32 sliderThumb       = 0xFFFFFFFE;

        static constexpr juce::uint32 listBackground    = 0xFF252525;
        static constexpr juce::uint32 listRowAlt        = 0xFF2A2A2A;
        static constexpr juce::uint32 listSelection     = 0xFF404040;

        static constexpr juce::uint32 tabBackground     = 0xFF606060;
        static constexpr juce::uint32 tabSelected       = 0xFF4A90D9;
        static constexpr juce::uint32 tabButtonNormal   = 0xFF3A3A3A;
        static constexpr juce::uint32 tabButtonSelected = 0xFF505050;
        static constexpr juce::uint32 tabTextNormal     = 0xFF909090;
        static constexpr juce::uint32 tabTextSelected   = 0xFFFFFFFE;
    };

    class PluginLookAndFeel  : public juce::LookAndFeel_V4
    {
    public:
        PluginLookAndFeel();
        ~PluginLookAndFeel() override = default;

        juce::Font getLabelFont (juce::Label&) override;
        juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Font getPopupMenuFont() override;

    private:
        void setupColours();

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginLookAndFeel)
    };
}
