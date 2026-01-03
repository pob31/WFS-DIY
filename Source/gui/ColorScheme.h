#pragma once

#include <JuceHeader.h>

/**
 * Centralized Color Scheme System
 *
 * Three schemes: Default (dark gray), OLED Black, Light
 * Components access colors via ColorScheme::get().colorName
 */
namespace ColorScheme
{
    enum class Theme
    {
        Default = 0,   // Current dark gray theme
        OLEDBlack,     // Pure black backgrounds for OLED displays
        Light          // Daytime/light theme
    };

    /**
     * Color Palette - semantic color names for the application
     */
    struct Palette
    {
        // Primary backgrounds
        juce::Colour background;          // Main component backgrounds
        juce::Colour backgroundAlt;       // Alternate backgrounds
        juce::Colour surfaceCard;         // Card/panel surfaces

        // Chrome areas (status bar, tab bar, footer)
        juce::Colour chromeBackground;    // StatusBar, TabbedComponent bar
        juce::Colour chromeSurface;       // Footer buttons area
        juce::Colour chromeDivider;       // Separator lines

        // Interactive elements
        juce::Colour buttonNormal;        // Normal button background
        juce::Colour buttonHover;         // Hover state
        juce::Colour buttonPressed;       // Pressed state
        juce::Colour buttonBorder;        // Button borders

        // Text colors
        juce::Colour textPrimary;         // Primary text
        juce::Colour textSecondary;       // Secondary/dimmed text
        juce::Colour textDisabled;        // Disabled state

        // Functional accents (same across themes)
        juce::Colour accentBlue;          // Blue accent buttons
        juce::Colour accentRed;           // Red/store buttons
        juce::Colour accentGreen;         // Green/load buttons
        juce::Colour accentGreenDark;     // Darker green

        // Sliders and dials (track backgrounds)
        juce::Colour sliderTrackBg;       // Track background
        juce::Colour sliderThumb;         // Slider thumb

        // ListBox and selection
        juce::Colour listBackground;      // ListBox background
        juce::Colour listRowAlt;          // Alternating row
        juce::Colour listSelection;       // Selected row

        // Tab bar specific
        juce::Colour tabBackground;       // Tab bar background
        juce::Colour tabSelected;         // Selected tab indicator
        juce::Colour tabButtonNormal;     // Unselected tab button background
        juce::Colour tabButtonSelected;   // Selected tab button background
        juce::Colour tabTextNormal;       // Unselected tab text
        juce::Colour tabTextSelected;     // Selected tab text
    };

    // Default Dark Gray Palette (current theme)
    inline const Palette DefaultPalette = {
        // Primary backgrounds
        juce::Colour(0xFF1E1E1E),  // background
        juce::Colour(0xFF252525),  // backgroundAlt
        juce::Colour(0xFF2A2A2A),  // surfaceCard

        // Chrome
        juce::Colours::darkgrey,   // chromeBackground
        juce::Colour(0xFF252525),  // chromeSurface
        juce::Colour(0xFF404040),  // chromeDivider

        // Buttons
        juce::Colour(0xFF2A2A2A),  // buttonNormal
        juce::Colour(0xFF353535),  // buttonHover
        juce::Colour(0xFF404040),  // buttonPressed
        juce::Colour(0xFF606060),  // buttonBorder

        // Text
        juce::Colours::white,      // textPrimary
        juce::Colour(0xFFAAAAAA),  // textSecondary
        juce::Colour(0xFF808080),  // textDisabled

        // Accents
        juce::Colour(0xFF33668C),  // accentBlue
        juce::Colour(0xFF8C3333),  // accentRed
        juce::Colour(0xFF338C33),  // accentGreen
        juce::Colour(0xFF266626),  // accentGreenDark

        // Sliders
        juce::Colour(0xFF1E1E1E),  // sliderTrackBg
        juce::Colours::white,      // sliderThumb - white to match text

        // List
        juce::Colour(0xFF252525),  // listBackground
        juce::Colour(0xFF2A2A2A),  // listRowAlt
        juce::Colour(0xFF404040),  // listSelection

        // Tab bar
        juce::Colours::darkgrey,   // tabBackground
        juce::Colour(0xFF4A90D9),  // tabSelected
        juce::Colour(0xFF3A3A3A),  // tabButtonNormal - darker, unselected
        juce::Colour(0xFF505050),  // tabButtonSelected - lighter, selected
        juce::Colour(0xFF909090),  // tabTextNormal - dimmed text
        juce::Colours::white,      // tabTextSelected - bright text
    };

    // OLED Black Palette (pure black for power savings)
    inline const Palette OLEDBlackPalette = {
        // Primary backgrounds - pure black
        juce::Colour(0xFF000000),  // background - PURE BLACK
        juce::Colour(0xFF0A0A0A),  // backgroundAlt
        juce::Colour(0xFF121212),  // surfaceCard

        // Chrome - darker than default
        juce::Colour(0xFF0D0D0D),  // chromeBackground
        juce::Colour(0xFF0A0A0A),  // chromeSurface
        juce::Colour(0xFF2A2A2A),  // chromeDivider

        // Buttons - darker
        juce::Colour(0xFF1A1A1A),  // buttonNormal
        juce::Colour(0xFF252525),  // buttonHover
        juce::Colour(0xFF303030),  // buttonPressed
        juce::Colour(0xFF404040),  // buttonBorder

        // Text - slightly warmer white for contrast
        juce::Colour(0xFFE8E8E8),  // textPrimary
        juce::Colour(0xFF909090),  // textSecondary
        juce::Colour(0xFF606060),  // textDisabled

        // Accents - same as default (functional colors)
        juce::Colour(0xFF33668C),  // accentBlue
        juce::Colour(0xFF8C3333),  // accentRed
        juce::Colour(0xFF338C33),  // accentGreen
        juce::Colour(0xFF266626),  // accentGreenDark

        // Sliders
        juce::Colour(0xFF000000),  // sliderTrackBg
        juce::Colour(0xFFE8E8E8),  // sliderThumb - near white to match text

        // List
        juce::Colour(0xFF0A0A0A),  // listBackground
        juce::Colour(0xFF121212),  // listRowAlt
        juce::Colour(0xFF252525),  // listSelection

        // Tab bar
        juce::Colour(0xFF0D0D0D),  // tabBackground
        juce::Colour(0xFF4A90D9),  // tabSelected
        juce::Colour(0xFF1A1A1A),  // tabButtonNormal - darker, unselected
        juce::Colour(0xFF303030),  // tabButtonSelected - lighter, selected
        juce::Colour(0xFF707070),  // tabTextNormal - dimmed text
        juce::Colour(0xFFE8E8E8),  // tabTextSelected - bright text
    };

    // Light Palette (daytime use)
    inline const Palette LightPalette = {
        // Primary backgrounds - light
        juce::Colour(0xFFF5F5F5),  // background
        juce::Colour(0xFFEAEAEA),  // backgroundAlt
        juce::Colour(0xFFFFFFFF),  // surfaceCard

        // Chrome
        juce::Colour(0xFFE0E0E0),  // chromeBackground
        juce::Colour(0xFFEAEAEA),  // chromeSurface
        juce::Colour(0xFFBDBDBD),  // chromeDivider

        // Buttons
        juce::Colour(0xFFE0E0E0),  // buttonNormal
        juce::Colour(0xFFD0D0D0),  // buttonHover
        juce::Colour(0xFFC0C0C0),  // buttonPressed
        juce::Colour(0xFF9E9E9E),  // buttonBorder

        // Text - dark for light backgrounds
        juce::Colour(0xFF212121),  // textPrimary
        juce::Colour(0xFF616161),  // textSecondary
        juce::Colour(0xFF9E9E9E),  // textDisabled

        // Accents - brightened for light background
        juce::Colour(0xFF4A90D9),  // accentBlue
        juce::Colour(0xFFD32F2F),  // accentRed
        juce::Colour(0xFF388E3C),  // accentGreen
        juce::Colour(0xFF2E7D32),  // accentGreenDark

        // Sliders
        juce::Colour(0xFFE0E0E0),  // sliderTrackBg
        juce::Colour(0xFF212121),  // sliderThumb - dark to match text

        // List
        juce::Colour(0xFFEEEEEE),  // listBackground
        juce::Colour(0xFFE0E0E0),  // listRowAlt
        juce::Colour(0xFFBBDEFB),  // listSelection (light blue)

        // Tab bar
        juce::Colour(0xFFE0E0E0),  // tabBackground
        juce::Colour(0xFF1976D2),  // tabSelected
        juce::Colour(0xFFD0D0D0),  // tabButtonNormal - lighter, unselected
        juce::Colour(0xFFFFFFFF),  // tabButtonSelected - white, selected
        juce::Colour(0xFF757575),  // tabTextNormal - dimmed text
        juce::Colour(0xFF212121),  // tabTextSelected - dark text
    };

    /**
     * ColorSchemeManager - Singleton to manage current theme
     */
    class Manager
    {
    public:
        static Manager& getInstance()
        {
            static Manager instance;
            return instance;
        }

        const Palette& getCurrentPalette() const { return *currentPalette; }
        Theme getCurrentTheme() const { return currentTheme; }

        void setTheme(Theme theme)
        {
            if (theme == currentTheme)
                return;

            currentTheme = theme;
            switch (theme)
            {
                case Theme::OLEDBlack: currentPalette = &OLEDBlackPalette; break;
                case Theme::Light:     currentPalette = &LightPalette; break;
                default:               currentPalette = &DefaultPalette; break;
            }
            notifyListeners();
        }

        void setTheme(int themeIndex)
        {
            setTheme(static_cast<Theme>(juce::jlimit(0, 2, themeIndex)));
        }

        // Listener interface for theme changes
        class Listener
        {
        public:
            virtual ~Listener() = default;
            virtual void colorSchemeChanged() = 0;
        };

        void addListener(Listener* listener) { listeners.add(listener); }
        void removeListener(Listener* listener) { listeners.remove(listener); }

    private:
        Manager() : currentPalette(&DefaultPalette), currentTheme(Theme::Default) {}

        void notifyListeners()
        {
            listeners.call([](Listener& l) { l.colorSchemeChanged(); });
        }

        const Palette* currentPalette;
        Theme currentTheme;
        juce::ListenerList<Listener> listeners;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Manager)
    };

    // Convenience function for accessing current palette
    inline const Palette& get() { return Manager::getInstance().getCurrentPalette(); }

    // Convenience function for getting theme as int (for ComboBox)
    inline int getThemeIndex() { return static_cast<int>(Manager::getInstance().getCurrentTheme()); }

} // namespace ColorScheme
