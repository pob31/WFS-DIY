#pragma once

/**
 * StreamDeckRenderer — Renders button and LCD strip images for the Stream Deck+.
 *
 * Uses juce::Graphics to draw 120x120 button images and 200x100 LCD zones.
 * Designed to match the WFS-DIY color scheme conventions:
 *   Blue = level, Teal = spatial, Yellow = time, Violet = effects
 */

#include <JuceHeader.h>
#include "StreamDeckPage.h"
#include "StreamDeckDevice.h"

class StreamDeckRenderer
{
public:
    //==========================================================================
    // Configuration
    //==========================================================================

    /** Font size for section selector (top row) button labels. */
    float sectionFontSize = 14.0f;

    /** Font size for bottom-row button labels. */
    float buttonFontSize = 13.0f;

    /** Font size for parameter name on LCD strip. */
    float lcdNameFontSize = 13.0f;

    /** Font size for parameter value on LCD strip. */
    float lcdValueFontSize = 20.0f;

    /** Background color for inactive section buttons. */
    juce::Colour sectionInactiveBackground { 0xFF2A2A2A };

    /** Background color for the active section button. */
    juce::Colour sectionActiveBackground { 0xFF4A90D9 };

    /** Text color for button labels. */
    juce::Colour textColour = juce::Colours::white;

    /** Background color for LCD strip zones. */
    juce::Colour lcdBackground { 0xFF1A1A1A };

    /** Text color for parameter names on LCD. */
    juce::Colour lcdNameColour { 0xFFAAAAAA };

    /** Text color for parameter values on LCD. */
    juce::Colour lcdValueColour = juce::Colours::white;

    /** Highlight color for combobox selection mode. */
    juce::Colour comboHighlightColour { 0xFF4A90D9 };

    //==========================================================================
    // Section Selector Buttons (Top Row: 0-3)
    //==========================================================================

    /** Render a section selector button image.
        @param section       The section data
        @param isActive      Whether this section is currently selected
        @param sectionIndex  Index of this section (0-3)
        @param numSections   Total number of sections on the page
        @return 120x120 JUCE Image ready to send to the device
    */
    juce::Image renderSectionButton (const StreamDeckSection& section,
                                     bool isActive,
                                     int sectionIndex,
                                     int numSections) const
    {
        juce::Image img (juce::Image::RGB,
                         StreamDeckDevice::BUTTON_IMAGE_WIDTH,
                         StreamDeckDevice::BUTTON_IMAGE_HEIGHT, true);
        juce::Graphics g (img);

        // Background
        juce::Colour bg = isActive ? sectionActiveBackground : sectionInactiveBackground;
        if (sectionIndex < numSections)
            bg = isActive ? section.sectionColour : section.sectionColour.withAlpha (0.3f);

        g.fillAll (bg);

        // Active indicator bar at bottom
        if (isActive)
        {
            g.setColour (section.sectionColour.brighter (0.3f));
            g.fillRect (0, StreamDeckDevice::BUTTON_IMAGE_HEIGHT - 4,
                        StreamDeckDevice::BUTTON_IMAGE_WIDTH, 4);
        }

        // Section name text (centered, multi-line if needed)
        if (sectionIndex < numSections && section.sectionName.isNotEmpty())
        {
            g.setColour (textColour);
            g.setFont (juce::Font (sectionFontSize));

            juce::Rectangle<int> textArea (8, 8,
                                           StreamDeckDevice::BUTTON_IMAGE_WIDTH - 16,
                                           StreamDeckDevice::BUTTON_IMAGE_HEIGHT - 16);
            g.drawFittedText (section.sectionName, textArea,
                              juce::Justification::centred, 3);
        }

        return img;
    }

    //==========================================================================
    // Context Buttons (Bottom Row: 4-7)
    //==========================================================================

    /** Render a context button image.
        @param binding  The button binding data
        @return 120x120 JUCE Image
    */
    juce::Image renderContextButton (const ButtonBinding& binding) const
    {
        juce::Image img (juce::Image::RGB,
                         StreamDeckDevice::BUTTON_IMAGE_WIDTH,
                         StreamDeckDevice::BUTTON_IMAGE_HEIGHT, true);
        juce::Graphics g (img);

        if (! binding.isValid())
        {
            g.fillAll (juce::Colour (0xFF1A1A1A));
            return img;
        }

        // Determine state and background color
        bool isOn = false;
        if (binding.type == ButtonBinding::Toggle && binding.getState)
            isOn = binding.getState();

        juce::Colour bg = isOn ? binding.activeColour : binding.colour;
        g.fillAll (bg);

        // Border
        g.setColour (bg.brighter (0.2f));
        g.drawRect (0, 0, StreamDeckDevice::BUTTON_IMAGE_WIDTH,
                    StreamDeckDevice::BUTTON_IMAGE_HEIGHT, 2);

        // Label text
        if (binding.label.isNotEmpty())
        {
            g.setColour (textColour);
            g.setFont (juce::Font (buttonFontSize));

            juce::Rectangle<int> textArea (6, 6,
                                           StreamDeckDevice::BUTTON_IMAGE_WIDTH - 12,
                                           StreamDeckDevice::BUTTON_IMAGE_HEIGHT - 12);
            g.drawFittedText (binding.label, textArea,
                              juce::Justification::centred, 3);
        }

        // Toggle state indicator dot
        if (binding.type == ButtonBinding::Toggle)
        {
            juce::Colour dotColour = isOn ? juce::Colours::white : juce::Colour (0xFF666666);
            g.setColour (dotColour);
            g.fillEllipse (StreamDeckDevice::BUTTON_IMAGE_WIDTH / 2.0f - 4.0f,
                           StreamDeckDevice::BUTTON_IMAGE_HEIGHT - 16.0f,
                           8.0f, 8.0f);
        }

        return img;
    }

    //==========================================================================
    // LCD Strip Zones (one per dial, 200x100 each)
    //==========================================================================

    /** Render an LCD zone for a dial parameter display.
        @param binding  The dial binding data
        @return 200x100 JUCE Image
    */
    juce::Image renderLcdZone (const DialBinding& binding) const
    {
        juce::Image img (juce::Image::RGB,
                         StreamDeckDevice::LCD_ZONE_WIDTH,
                         StreamDeckDevice::LCD_STRIP_HEIGHT, true);
        juce::Graphics g (img);

        g.fillAll (lcdBackground);

        if (! binding.isValid())
            return img;

        // Separator lines between zones
        g.setColour (juce::Colour (0xFF333333));
        g.drawVerticalLine (0, 0.0f, static_cast<float> (StreamDeckDevice::LCD_STRIP_HEIGHT));
        g.drawVerticalLine (StreamDeckDevice::LCD_ZONE_WIDTH - 1, 0.0f,
                            static_cast<float> (StreamDeckDevice::LCD_STRIP_HEIGHT));

        int zoneW = StreamDeckDevice::LCD_ZONE_WIDTH;
        int zoneH = StreamDeckDevice::LCD_STRIP_HEIGHT;

        // Parameter name (top portion)
        g.setColour (lcdNameColour);
        g.setFont (juce::Font (lcdNameFontSize));
        g.drawFittedText (binding.paramName,
                          4, 4, zoneW - 8, 24,
                          juce::Justification::centred, 1);

        // Parameter value (center, larger font)
        g.setColour (lcdValueColour);
        g.setFont (juce::Font (lcdValueFontSize).boldened());
        g.drawFittedText (binding.formatValueWithUnit(),
                          4, 28, zoneW - 8, 40,
                          juce::Justification::centred, 1);

        // Value bar (bottom portion) — visual indicator of normalized position
        if (binding.type != DialBinding::ComboBox)
        {
            float normalized = 0.0f;
            float current = binding.getValue();

            if (binding.isExponential && binding.minValue > 0.0f)
                normalized = std::log (current / binding.minValue)
                             / std::log (binding.maxValue / binding.minValue);
            else if (binding.maxValue != binding.minValue)
                normalized = (current - binding.minValue) / (binding.maxValue - binding.minValue);

            normalized = juce::jlimit (0.0f, 1.0f, normalized);

            int barY = zoneH - 14;
            int barH = 6;
            int barMargin = 12;
            int barWidth = zoneW - barMargin * 2;

            // Track background
            g.setColour (juce::Colour (0xFF333333));
            g.fillRoundedRectangle (static_cast<float> (barMargin), static_cast<float> (barY),
                                    static_cast<float> (barWidth), static_cast<float> (barH),
                                    3.0f);

            // Filled portion
            g.setColour (sectionActiveBackground);
            g.fillRoundedRectangle (static_cast<float> (barMargin), static_cast<float> (barY),
                                    barWidth * normalized, static_cast<float> (barH),
                                    3.0f);
        }

        return img;
    }

    /** Render an LCD zone in ComboBox selection mode.
        @param binding       The dial binding
        @param selectedIndex The currently highlighted option index
        @return 200x100 JUCE Image
    */
    juce::Image renderLcdZoneComboMode (const DialBinding& binding, int selectedIndex) const
    {
        juce::Image img (juce::Image::RGB,
                         StreamDeckDevice::LCD_ZONE_WIDTH,
                         StreamDeckDevice::LCD_STRIP_HEIGHT, true);
        juce::Graphics g (img);

        g.fillAll (lcdBackground);

        if (! binding.isValid() || binding.comboOptions.isEmpty())
            return img;

        int zoneW = StreamDeckDevice::LCD_ZONE_WIDTH;

        // Title
        g.setColour (lcdNameColour);
        g.setFont (juce::Font (lcdNameFontSize));
        g.drawFittedText (binding.paramName, 4, 2, zoneW - 8, 18,
                          juce::Justification::centred, 1);

        // Show up to 3 options centered on the selected one
        int lineHeight = 22;
        int startY = 24;
        int startIndex = juce::jmax (0, selectedIndex - 1);
        int endIndex = juce::jmin (binding.comboOptions.size(), startIndex + 3);
        if (endIndex - startIndex < 3 && startIndex > 0)
            startIndex = juce::jmax (0, endIndex - 3);

        for (int i = startIndex; i < endIndex; ++i)
        {
            int y = startY + (i - startIndex) * lineHeight;

            if (i == selectedIndex)
            {
                g.setColour (comboHighlightColour);
                g.fillRoundedRectangle (6.0f, static_cast<float> (y),
                                        static_cast<float> (zoneW - 12),
                                        static_cast<float> (lineHeight - 2), 4.0f);
                g.setColour (juce::Colours::white);
            }
            else
            {
                g.setColour (lcdNameColour);
            }

            g.setFont (juce::Font (14.0f));
            g.drawFittedText (binding.comboOptions[i],
                              10, y + 1, zoneW - 20, lineHeight - 2,
                              juce::Justification::centred, 1);
        }

        // Scroll indicators
        if (startIndex > 0)
        {
            g.setColour (lcdNameColour);
            g.drawText (juce::CharPointer_UTF8 ("\xe2\x96\xb2"),  // ▲
                        zoneW - 20, startY - 2, 16, 14,
                        juce::Justification::centred);
        }
        if (endIndex < binding.comboOptions.size())
        {
            g.setColour (lcdNameColour);
            g.drawText (juce::CharPointer_UTF8 ("\xe2\x96\xbc"),  // ▼
                        zoneW - 20, startY + 3 * lineHeight - 14, 16, 14,
                        juce::Justification::centred);
        }

        return img;
    }

    //==========================================================================
    // Full Page Rendering Helpers
    //==========================================================================

    /** Render all 8 button images for a page and send them to the device. */
    void renderAndSendAllButtons (StreamDeckDevice& device, const StreamDeckPage& page) const
    {
        // Top row: section selectors (buttons 0-3)
        for (int i = 0; i < 4; ++i)
        {
            bool isActive = (i == page.activeSectionIndex);
            auto img = renderSectionButton (page.sections[i], isActive, i, page.numSections);
            device.setButtonImage (i, img);
        }

        // Bottom row: context buttons (buttons 4-7)
        const auto& section = page.getActiveSection();
        for (int i = 0; i < 4; ++i)
        {
            auto img = renderContextButton (section.buttons[i]);
            device.setButtonImage (4 + i, img);
        }
    }

    /** Render all 4 LCD zones for the active section and send to device. */
    void renderAndSendAllLcdZones (StreamDeckDevice& device, const StreamDeckPage& page) const
    {
        const auto& section = page.getActiveSection();
        for (int i = 0; i < 4; ++i)
        {
            auto img = renderLcdZone (section.dials[i]);
            device.setLcdZoneImage (i, img);
        }
    }

    /** Render and send everything (all buttons + all LCD zones). */
    void renderAndSendFullPage (StreamDeckDevice& device, const StreamDeckPage& page) const
    {
        renderAndSendAllButtons (device, page);
        renderAndSendAllLcdZones (device, page);
    }
};
