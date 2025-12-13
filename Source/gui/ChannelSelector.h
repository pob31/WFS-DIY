#pragma once

#include <JuceHeader.h>

/**
 * Channel Selector Overlay Component
 * A reusable grid-based channel selector that opens as an overlay.
 * Used for Input, Output, Cluster, and Reverb tabs.
 */
class ChannelSelectorOverlay : public juce::Component
{
public:
    ChannelSelectorOverlay(int numChannels, int currentChannel, std::function<void(int)> onChannelSelected)
        : totalChannels(numChannels),
          selectedChannel(currentChannel),
          onSelect(std::move(onChannelSelected))
    {
        setOpaque(false);
        setAlwaysOnTop(true);

        // Calculate grid dimensions (prefer 8 columns)
        numColumns = juce::jmin(8, totalChannels);
        numRows = (totalChannels + numColumns - 1) / numColumns;

        // Create channel buttons
        for (int i = 1; i <= totalChannels; ++i)
        {
            auto* btn = new juce::TextButton(juce::String(i));
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
        // Semi-transparent background overlay
        g.fillAll(juce::Colours::black.withAlpha(0.85f));

        // Draw title
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions().withHeight(18.0f).withStyle("Bold"));
        g.drawText("Select Channel", getLocalBounds().removeFromTop(40), juce::Justification::centred);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(20);

        // Title area
        bounds.removeFromTop(40);

        // Close button in top-right
        closeButton.setBounds(getWidth() - 50, 10, 40, 30);

        // Calculate button size based on available space
        const int spacing = 4;
        const int availableWidth = bounds.getWidth() - (spacing * (numColumns - 1));
        const int availableHeight = bounds.getHeight() - (spacing * (numRows - 1));
        const int buttonWidth = availableWidth / numColumns;
        const int buttonHeight = juce::jmin(40, availableHeight / numRows);

        // Center the grid
        const int gridWidth = numColumns * buttonWidth + (numColumns - 1) * spacing;
        const int gridHeight = numRows * buttonHeight + (numRows - 1) * spacing;
        const int startX = bounds.getX() + (bounds.getWidth() - gridWidth) / 2;
        const int startY = bounds.getY() + (bounds.getHeight() - gridHeight) / 2;

        for (int i = 0; i < channelButtons.size(); ++i)
        {
            int row = i / numColumns;
            int col = i % numColumns;
            int x = startX + col * (buttonWidth + spacing);
            int y = startY + row * (buttonHeight + spacing);

            channelButtons[i]->setBounds(x, y, buttonWidth, buttonHeight);

            // Highlight selected channel
            if (i + 1 == selectedChannel)
            {
                channelButtons[i]->setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF4080FF));
                channelButtons[i]->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
            }
            else
            {
                channelButtons[i]->setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2A2A2A));
                channelButtons[i]->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
            }
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        // Close if clicking outside the grid area
        bool clickedOnButton = false;
        for (auto* btn : channelButtons)
        {
            if (btn->getBounds().contains(e.getPosition()))
            {
                clickedOnButton = true;
                break;
            }
        }
        if (!clickedOnButton && !closeButton.getBounds().contains(e.getPosition()))
        {
            if (onSelect)
                onSelect(selectedChannel);
        }
    }

private:
    int totalChannels;
    int selectedChannel;
    int numColumns = 8;
    int numRows = 1;
    std::function<void(int)> onSelect;

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

            auto overlay = std::make_unique<ChannelSelectorOverlay>(
                numChannels,
                currentChannel,
                [safeThis, safeParent](int selected) {
                    // Defer all work to after the current callback finishes.
                    // This prevents accessing invalidated memory if components
                    // are recreated during setSelectedChannel() callbacks.
                    juce::MessageManager::callAsync([safeThis, safeParent, selected]() {
                        // Check if parent is still valid for cleanup
                        if (safeParent == nullptr)
                            return;

                        auto* parent = safeParent.getComponent();

                        // Remove overlay FIRST (before any callbacks that might change state)
                        for (int i = parent->getNumChildComponents() - 1; i >= 0; --i)
                        {
                            if (auto* comp = dynamic_cast<ChannelSelectorOverlay*>(parent->getChildComponent(i)))
                            {
                                parent->removeChildComponent(comp);
                                delete comp;
                                break;
                            }
                        }

                        // Update channel selection (may trigger callbacks)
                        if (safeThis != nullptr)
                            safeThis->setSelectedChannel(selected);

                        // Grab focus on the selector button itself - key events will
                        // propagate up to MainComponent through the component hierarchy
                        juce::MessageManager::callAsync([safeThis]() {
                            if (safeThis != nullptr)
                                safeThis->grabKeyboardFocus();
                        });
                    });
                }
            );

            overlay->setBounds(parent->getLocalBounds());
            parent->addAndMakeVisible(overlay.release());
        }
    }

    juce::String labelPrefix;
    juce::TextButton selectorButton;
    int numChannels = 64;
    int currentChannel = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelSelectorButton)
};
