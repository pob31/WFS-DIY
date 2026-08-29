#pragma once

#include <JuceHeader.h>
#include "ColorScheme.h"
#include "ColorUtilities.h"
#include "WfsLookAndFeel.h"
#include "../Localization/LocalizationManager.h"

/**
 * The input colour wheel, opened from the round swatch in the channel selector's popup.
 *
 * It deliberately does NOT live beside the collapsed channel button: that button is already
 * painted in the channel's colour, so a swatch next to it says the same thing twice. Inside
 * the grid the swatch sits where every other channel's colour is visible for comparison.
 *
 * The popup follows the app's own transient-UI idiom rather than juce::CallOutBox (which
 * appears nowhere in this codebase): a transparent click-catcher plus a card, both added to
 * the TOP-LEVEL component so they can overhang the tab, dismissed by a click anywhere
 * outside. Same construction as ChannelSelectorButton::showOverlay and LightpadZoneOverlay,
 * including the clamp-to-window and flip-above-on-overflow behaviour.
 */
namespace WfsInputColourPicker
{
    inline int sc (float ref) { return juce::jmax (1, juce::roundToInt (ref * WfsLookAndFeel::uiScale)); }

    //==========================================================================
    /** Transparent full-window click catcher. Sits UNDER the card, so a click anywhere
        else dismisses. Mirrors ChannelSelectorBackdrop. */
    class Backdrop : public juce::Component
    {
    public:
        explicit Backdrop (std::function<void()> onDismissIn)
            : onDismiss (std::move (onDismissIn))
        {
            setOpaque (false);
            setInterceptsMouseClicks (true, false);
            setAlwaysOnTop (true);
        }

        void paint (juce::Graphics&) override {}

        void mouseDown (const juce::MouseEvent&) override
        {
            if (onDismiss)
                onDismiss();
        }

    private:
        std::function<void()> onDismiss;
    };

    //==========================================================================
    /** The card: a colour wheel plus an "Auto" button that returns the channel to the
        colour derived from its number. */
    class Overlay : public juce::Component,
                    private juce::ChangeListener
    {
    public:
        Overlay (juce::Colour initial,
                 std::function<void (juce::Colour)> onColourIn,
                 std::function<void()> onAutoIn,
                 std::function<void()> onCloseIn)
            : onColour (std::move (onColourIn)),
              onAuto (std::move (onAutoIn)),
              onClose (std::move (onCloseIn))
        {
            setAlwaysOnTop (true);

            selector = std::make_unique<juce::ColourSelector> (
                juce::ColourSelector::showColourspace | juce::ColourSelector::showSliders,
                sc (4), sc (6));
            selector->setCurrentColour (initial, juce::dontSendNotification);
            selector->addChangeListener (this);
            addAndMakeVisible (*selector);

            autoButton.setButtonText (LOC ("inputs.buttons.colourAuto"));
            autoButton.onClick = [this]() { if (onAuto) onAuto(); };
            addAndMakeVisible (autoButton);

            closeButton.setButtonText (LOC ("common.close"));
            closeButton.onClick = [this]() { if (onClose) onClose(); };
            addAndMakeVisible (closeButton);
        }

        ~Overlay() override
        {
            if (selector != nullptr)
                selector->removeChangeListener (this);
        }

        static juce::Point<int> getRequiredSize()
        {
            return { sc (260), sc (300) };
        }

        void paint (juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();

            juce::DropShadow (juce::Colours::black.withAlpha (0.5f), sc (12), {}).drawForRectangle (g, getLocalBounds());

            g.setColour (ColorScheme::get().surfaceCard);
            g.fillRoundedRectangle (bounds, 8.0f);
            g.setColour (juce::Colour (0xFF505050));
            g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, 1.0f);

            g.setColour (ColorScheme::get().textPrimary);
            g.setFont (juce::FontOptions (juce::jmax (11.0f, 14.0f * WfsLookAndFeel::uiScale)));
            g.drawText (LOC ("inputs.dialogs.selectColour"),
                        getLocalBounds().removeFromTop (sc (28)).reduced (sc (12), 0),
                        juce::Justification::centredLeft);
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced (sc (12));
            area.removeFromTop (sc (22));                       // title

            auto buttons = area.removeFromBottom (sc (28));
            autoButton.setBounds (buttons.removeFromLeft (sc (100)));
            closeButton.setBounds (buttons.removeFromRight (sc (80)));

            area.removeFromBottom (sc (8));
            if (selector != nullptr)
                selector->setBounds (area);
        }

    private:
        void changeListenerCallback (juce::ChangeBroadcaster*) override
        {
            if (onColour && selector != nullptr)
                onColour (selector->getCurrentColour());
        }

        std::unique_ptr<juce::ColourSelector> selector;
        juce::TextButton autoButton, closeButton;
        std::function<void (juce::Colour)> onColour;
        std::function<void()> onAuto;
        std::function<void()> onClose;
    };

    //==========================================================================
    /** Opens the wheel anchored under `anchor`, as a card plus a click-catcher added to the
        top-level component.

        A free function rather than a component, because the swatch that opens it lives
        inside the channel-selector popup: the picker has to be able to hang off any
        component, and to survive above a popup that is itself always-on-top.

        `onGestureStart` is called once, here, so a drag across the wheel collapses into a
        single undo step instead of one per change message.
    */
    inline void show (juce::Component& anchor,
                      juce::Colour initial,
                      std::function<void (juce::Colour)> onColour,
                      std::function<void()> onAuto,
                      std::function<void()> onGestureStart = {})
    {
        auto* parent = anchor.getTopLevelComponent();
        if (parent == nullptr)
            return;

        if (onGestureStart)
            onGestureStart();

        juce::Component::SafePointer<juce::Component> safeParent = parent;

        auto dismiss = [safeParent]()
        {
            juce::MessageManager::callAsync ([safeParent]()
            {
                if (auto* p = safeParent.getComponent())
                {
                    for (int i = p->getNumChildComponents(); --i >= 0;)
                    {
                        auto* child = p->getChildComponent (i);
                        // Only OUR two classes: the channel-selector popup underneath uses
                        // its own types and must survive, so its grid stays visible for
                        // comparison while a colour is being chosen.
                        if (dynamic_cast<Overlay*> (child) != nullptr
                            || dynamic_cast<Backdrop*> (child) != nullptr)
                        {
                            p->removeChildComponent (child);
                            delete child;
                        }
                    }
                }
            });
        };

        auto backdrop = std::make_unique<Backdrop> (dismiss);
        backdrop->setBounds (parent->getLocalBounds());
        parent->addAndMakeVisible (backdrop.release());

        auto overlay = std::make_unique<Overlay> (
            initial,
            std::move (onColour),
            [onAuto, dismiss]() { if (onAuto) onAuto(); dismiss(); },
            dismiss);

        const auto size = Overlay::getRequiredSize();
        auto anchorArea = parent->getLocalArea (&anchor, anchor.getLocalBounds());

        int x = anchorArea.getX();
        int y = anchorArea.getBottom() + sc (4);

        const auto pb = parent->getLocalBounds();
        if (x + size.x > pb.getRight())  x = pb.getRight() - size.x;
        if (x < 0)                        x = 0;
        if (y + size.y > pb.getBottom())  y = anchorArea.getY() - size.y - sc (4);   // flip above
        if (y < 0)                        y = 0;

        overlay->setBounds (x, y, size.x, size.y);
        parent->addAndMakeVisible (overlay.release());
    }
}
