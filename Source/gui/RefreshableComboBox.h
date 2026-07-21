#pragma once

#include <JuceHeader.h>

/** ComboBox subclass that rescans content before showing the popup menu. */
class RefreshableComboBox : public juce::ComboBox
{
public:
    std::function<void()> onPopupAboutToShow;

    void showPopup() override
    {
        if (onPopupAboutToShow)
            onPopupAboutToShow();
        juce::ComboBox::showPopup();
    }
};
