#pragma once

#include <JuceHeader.h>
#include "ColorUtilities.h"
#include "ColorScheme.h"
#include "WfsLookAndFeel.h"
#include "../Localization/LocalizationManager.h"

// Forward declaration
class ChannelSelectorOverlay;

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
                          std::function<juce::Colour(int)> textColorProvider = nullptr)
        : totalChannels(numChannels),
          selectedChannel(currentChannel),
          onSelect(std::move(onChannelSelected)),
          getChannelColor(std::move(channelColorProvider)),
          getChannelName(std::move(channelNameProvider)),
          getTextColor(std::move(textColorProvider))
    {
        setOpaque(false);
        setAlwaysOnTop(true);

        // Calculate adaptive grid dimensions - favor rows over columns
        // since buttons are wider (60) than tall (40)
        calculateGridDimensions(totalChannels);

        // Create channel buttons
        for (int i = 1; i <= totalChannels; ++i)
        {
            // Get button text - show name if available, otherwise just number
            juce::String buttonText;
            if (getChannelName)
            {
                juce::String name = getChannelName(i);
                if (name.isNotEmpty())
                    buttonText = juce::String(i) + "\n" + name;
                else
                    buttonText = juce::String(i);
            }
            else
            {
                buttonText = juce::String(i);
            }

            auto* btn = new juce::TextButton(buttonText);
            btn->setClickingTogglesState(false);
            btn->onClick = [this, i]() {
                if (onSelect)
                    onSelect(i);
            };
            channelButtons.add(btn);
            addAndMakeVisible(btn);
        }

        // Close button
        closeButton.setButtonText("X");
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
        g.setColour(juce::Colour(0xFF2A2A2A));
        g.fillRoundedRectangle(bounds, 8.0f);

        // Border
        g.setColour(juce::Colour(0xFF505050));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);

        // Draw title
        g.setColour(ColorScheme::get().textPrimary);
        g.setFont(juce::FontOptions().withHeight(juce::jmax(10.0f, 14.0f * WfsLookAndFeel::uiScale)).withStyle("Bold"));
        g.drawText(LOC("inputs.dialogs.selectChannel"), padding, padding, getWidth() - padding * 2 - sc(30), titleHeight - padding,
                   juce::Justification::centredLeft);
    }

    void resized() override
    {
        // Close button in top-right
        closeButton.setBounds(getWidth() - padding - sc(24), padding, sc(24), sc(20));

        // Position buttons in grid below title
        const int startX = padding;
        const int startY = titleHeight;

        for (int i = 0; i < channelButtons.size(); ++i)
        {
            int row = i / numColumns;
            int col = i % numColumns;
            int x = startX + col * (buttonWidth + spacing);
            int y = startY + row * (buttonHeight + spacing);

            channelButtons[i]->setBounds(x, y, buttonWidth, buttonHeight);

            // Get color for this channel
            juce::Colour buttonColor;
            if (getChannelColor)
            {
                // Use custom color from provider
                buttonColor = getChannelColor(i + 1);

                // If this is the selected channel, brighten it slightly
                if (i + 1 == selectedChannel)
                    buttonColor = buttonColor.brighter(0.3f);
            }
            else
            {
                // Default color scheme
                if (i + 1 == selectedChannel)
                    buttonColor = juce::Colour(0xFF4080FF);
                else
                    buttonColor = juce::Colour(0xFF3A3A3A);
            }

            // Get text color for this channel
            juce::Colour textColor = juce::Colours::white;  // Default to white for dark buttons
            if (getTextColor)
                textColor = getTextColor(i + 1);

            channelButtons[i]->setColour(juce::TextButton::buttonColourId, buttonColor);
            channelButtons[i]->setColour(juce::TextButton::textColourOffId, textColor);
        }
    }

    // Get the required size for this overlay based on channel count
    juce::Point<int> getRequiredSize() const
    {
        int width = padding * 2 + numColumns * buttonWidth + (numColumns - 1) * spacing;
        int height = titleHeight + numRows * buttonHeight + (numRows - 1) * spacing + padding;
        return {width, height};
    }

private:
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

    int totalChannels;
    int selectedChannel;
    int numColumns = 2;
    int numRows = 1;
    std::function<void(int)> onSelect;
    std::function<juce::Colour(int)> getChannelColor;
    std::function<juce::String(int)> getChannelName;
    std::function<juce::Colour(int)> getTextColor;

    juce::OwnedArray<juce::TextButton> channelButtons;
    juce::TextButton closeButton;

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
    }

    void setNumChannels(int num)
    {
        numChannels = juce::jmax(1, num);
        if (currentChannel > numChannels)
            setSelectedChannel(numChannels);
    }

    void setSelectedChannel(int channel)
    {
        currentChannel = juce::jlimit(1, numChannels, channel);
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
        currentChannel = juce::jlimit(1, numChannels, channel);
        updateButtonText();
        // Remove focus from the internal button to prevent Enter key from triggering overlay
        selectorButton.setWantsKeyboardFocus(false);
        if (onChannelChanged)
            onChannelChanged(currentChannel);
        // Re-enable keyboard focus after a short delay
        selectorButton.setWantsKeyboardFocus(true);
    }

    int getSelectedChannel() const { return currentChannel; }

    int getNumChannels() const { return numChannels; }

    void resized() override
    {
        selectorButton.setBounds(getLocalBounds());
    }

    std::function<void(int)> onChannelChanged;

private:
    void updateButtonText()
    {
        selectorButton.setButtonText(labelPrefix + " " + juce::String(currentChannel) + juce::String::fromUTF8(" ▼"));
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
                textColorProvider
            );

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

    juce::String labelPrefix;
    juce::TextButton selectorButton;
    int numChannels = 64;
    int currentChannel = 1;
    std::function<juce::Colour(int)> channelColorProvider;
    std::function<juce::String(int)> channelNameProvider;
    std::function<juce::Colour(int)> textColorProvider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelSelectorButton)
};
