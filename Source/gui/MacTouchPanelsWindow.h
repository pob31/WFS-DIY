#pragma once

#include <JuceHeader.h>

#if JUCE_MAC

#include "../../spatcore/controllers/touch/macos/MacHidTouchManager.h"
#include "WindowUtils.h"
#include "ColorScheme.h"
#include "../Localization/LocalizationManager.h"

#include <map>
#include <set>

/**
    "Touch the circle on this screen": assigns USB touch panels to displays.

    A panel's touch cannot say which display it lies on - that is the unknown.
    So every display gets an overlay with its circle in a different place, left
    to right in the order the displays are arranged; while the session runs the
    touch manager reports landings instead of routing them, and where a finger
    lands on its panel (as a fraction of the panel) names the circle it was
    aiming at, so the display. Every connected panel can be identified in one
    pass; the session ends when they all are, on a mouse click on any overlay,
    on Escape, or after 30 s.
*/
class TouchIdentifySession : private juce::Timer
{
public:
    using Manager = spatcore::controllers::MacHidTouchManager;

    TouchIdentifySession (Manager& m, std::function<void()> finished)
        : manager (m), onFinished (std::move (finished))
    {
        displays = Manager::getDisplays();
        std::sort (displays.begin(), displays.end(),
                   [] (const auto& a, const auto& b) { return a.bounds.getX() < b.bounds.getX(); });

        for (size_t i = 0; i < displays.size(); ++i)
        {
            const float targetX = ((float) i + 0.5f) / (float) displays.size();
            auto overlay = std::make_unique<Overlay> (*this, targetX);
            overlay->setBounds (displays[i].bounds);
            overlay->addToDesktop (juce::ComponentPeer::windowIsTemporary);
            overlay->setAlwaysOnTop (true);
            overlay->setVisible (true);
            overlays.push_back (std::move (overlay));
        }
        if (! overlays.empty())
            overlays.front()->grabKeyboardFocus();

        manager.onIdentifyTouch = [this] (const juce::String& key, float x, float) { handleTouch (key, x); };
        startTimer (30000);
    }

    ~TouchIdentifySession() override
    {
        manager.onIdentifyTouch = nullptr;
    }

private:
    struct Overlay : public juce::Component
    {
        Overlay (TouchIdentifySession& s, float x) : session (s), targetX (x)
        {
            setWantsKeyboardFocus (true);
        }

        void paint (juce::Graphics& g) override
        {
            const auto& cs = ColorScheme::get();
            g.fillAll (cs.background.withAlpha (0.88f));

            const auto b = getLocalBounds().toFloat();
            const auto centre = juce::Point<float> (b.getX() + targetX * b.getWidth(), b.getCentreY());
            const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.06f;

            g.setColour (cs.accentBlue);
            g.drawEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (centre), 4.0f);
            g.fillEllipse (juce::Rectangle<float> (r * 0.5f, r * 0.5f).withCentre (centre));

            g.setColour (cs.textPrimary);
            g.setFont (juce::FontOptions (juce::jmax (18.0f, b.getHeight() * 0.03f)));
            g.drawText (LOC ("touchScreens.identifyPrompt"),
                        juce::Rectangle<float> (b.getWidth(), r * 1.5f).withCentre ({ b.getCentreX(), centre.y - r * 2.2f }),
                        juce::Justification::centred);

            g.setColour (cs.textSecondary);
            g.setFont (juce::FontOptions (juce::jmax (14.0f, b.getHeight() * 0.018f)));
            g.drawText (LOC ("touchScreens.identifyCancel"),
                        b.withTrimmedTop (b.getHeight() * 0.88f), juce::Justification::centred);

            if (! assigned.isEmpty())
            {
                g.setColour (cs.accentGreen);
                g.setFont (juce::FontOptions (juce::jmax (18.0f, b.getHeight() * 0.025f)));
                g.drawText (LOC ("touchScreens.identifyAssigned") + " " + assigned.joinIntoString (", "),
                            juce::Rectangle<float> (getWidth(), r * 1.5f).withCentre ({ (float) getWidth() * 0.5f, centre.y + r * 2.2f }),
                            juce::Justification::centred);
            }
        }

        void mouseDown (const juce::MouseEvent&) override   { session.finishSoon (0); }

        bool keyPressed (const juce::KeyPress& k) override
        {
            if (k == juce::KeyPress::escapeKey)
                session.finishSoon (0);
            return true;
        }

        TouchIdentifySession& session;
        float targetX;
        juce::StringArray assigned;
    };

    void handleTouch (const juce::String& key, float panelX)
    {
        if (displays.empty() || finishing)
            return;

        size_t best = 0;
        for (size_t i = 1; i < displays.size(); ++i)
            if (std::abs (panelX - overlays[i]->targetX) < std::abs (panelX - overlays[best]->targetX))
                best = i;

        juce::String name = key;
        bool found = false;
        for (auto& p : manager.getPanels())
            if (p.key == key)
            {
                auto s = p.settings;
                s.displayId = displays[best].id;
                s.displayIndex = -1;
                manager.setPanelSettings (key, s);
                if (p.name.isNotEmpty())
                    name = p.name;
                found = true;
            }
        if (! found)
            return;

        identified.insert (key);
        overlays[best]->assigned.addIfNotAlreadyThere (name);
        overlays[best]->repaint();

        int connected = 0;
        for (auto& p : manager.getPanels())
            if (p.connected)
                ++connected;
        if ((int) identified.size() >= connected)
            finishSoon (900);   // long enough to read the confirmation
    }

    void finishSoon (int ms)
    {
        finishing = true;
        manager.onIdentifyTouch = nullptr;
        startTimer (juce::jmax (1, ms));
    }

    void timerCallback() override
    {
        stopTimer();
        manager.onIdentifyTouch = nullptr;
        // The owner destroys this session; never from inside its own callback.
        juce::MessageManager::callAsync ([cb = onFinished] { if (cb) cb(); });
    }

    Manager& manager;
    std::function<void()> onFinished;
    std::vector<Manager::DisplayInfo> displays;
    std::vector<std::unique_ptr<Overlay>> overlays;
    std::set<juce::String> identified;
    bool finishing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TouchIdentifySession)
};

//==============================================================================
/** One row per connected touch panel: its display, whether touches outside
    WFS-DIY drive the cursor, and whether it is opened exclusively. */
class MacTouchPanelsContent : public juce::Component,
                              private juce::AsyncUpdater
{
public:
    using Manager = spatcore::controllers::MacHidTouchManager;

    MacTouchPanelsContent (Manager& m, std::function<void()> identify)
        : manager (m), onIdentify (std::move (identify))
    {
        addAndMakeVisible (identifyButton);
        identifyButton.setButtonText (LOC ("touchScreens.identify"));
        identifyButton.onClick = [this] { if (onIdentify) onIdentify(); };

        addAndMakeVisible (hint);
        hint.setText (LOC ("touchScreens.identifyHint"), juce::dontSendNotification);
        hint.setColour (juce::Label::textColourId, ColorScheme::get().textSecondary);
        hint.setJustificationType (juce::Justification::topLeft);

        // Rebuilt asynchronously: a row's own combo box or toggle changes the
        // settings, which fires this listener - rebuilding at once would
        // delete that control inside its own callback.
        listenerToken = manager.addChangeListener ([this] { triggerAsyncUpdate(); });
        rebuild();
    }

    ~MacTouchPanelsContent() override
    {
        manager.removeChangeListener (listenerToken);
    }

    void paint (juce::Graphics& g) override
    {
        const auto& cs = ColorScheme::get();
        g.fillAll (cs.background);

        if (rows.empty())
        {
            g.setColour (cs.textSecondary);
            g.setFont (juce::FontOptions (15.0f));
            g.drawFittedText (LOC ("touchScreens.empty"), getLocalBounds().withTrimmedTop (110).reduced (16),
                              juce::Justification::centredTop, 3);
        }

        g.setColour (cs.chromeDivider);
        for (auto& row : rows)
            g.drawHorizontalLine (row->getY() - 6, 12.0f, (float) getWidth() - 12.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (12);
        auto top = area.removeFromTop (32);
        identifyButton.setBounds (top.removeFromLeft (200));
        hint.setBounds (area.removeFromTop (56).withTrimmedTop (6));
        area.removeFromTop (12);

        for (auto& row : rows)
        {
            row->setBounds (area.removeFromTop (Row::height));
            area.removeFromTop (14);
        }
    }

    int getIdealHeight() const { return 24 + 32 + 62 + 12 + juce::jmax (1, (int) rows.size()) * (Row::height + 14) + 20; }

private:
    struct Row : public juce::Component
    {
        static constexpr int height = 96;

        Row (Manager& m, const Manager::PanelInfo& p) : manager (m), info (p)
        {
            const auto& cs = ColorScheme::get();

            addAndMakeVisible (name);
            name.setText (p.name.isNotEmpty() ? p.name : p.key, juce::dontSendNotification);
            name.setFont (juce::FontOptions (16.0f, juce::Font::bold));

            addAndMakeVisible (status);
            status.setColour (juce::Label::textColourId, cs.textSecondary);
            status.setText (statusText(), juce::dontSendNotification);

            addAndMakeVisible (displayLabel);
            displayLabel.setText (LOC ("touchScreens.display"), juce::dontSendNotification);

            addAndMakeVisible (displayBox);
            displayBox.addItem (LOC ("touchScreens.automatic"), 1);
            displays = Manager::getDisplays();
            int selected = 1;
            for (size_t i = 0; i < displays.size(); ++i)
            {
                const auto& d = displays[i];
                displayBox.addItem ((d.name.isNotEmpty() ? d.name : LOC ("touchScreens.display")) + "  "
                                        + juce::String (d.bounds.getWidth()) + " x " + juce::String (d.bounds.getHeight())
                                        + (d.isMain ? "  (" + LOC ("touchScreens.main") + ")" : juce::String()),
                                    (int) i + 2);
                if (d.id == p.settings.displayId)
                    selected = (int) i + 2;
            }
            displayBox.setSelectedId (selected, juce::dontSendNotification);
            displayBox.onChange = [this]
            {
                auto s = info.settings;
                const int id = displayBox.getSelectedId();
                s.displayId = id >= 2 ? displays[(size_t) (id - 2)].id : juce::String();
                s.displayIndex = -1;
                manager.setPanelSettings (info.key, s);
            };

            addAndMakeVisible (cursorToggle);
            cursorToggle.setButtonText (LOC ("touchScreens.driveCursor"));
            cursorToggle.setToggleState (p.settings.drivePointer, juce::dontSendNotification);
            cursorToggle.onClick = [this]
            {
                auto s = info.settings;
                s.drivePointer = cursorToggle.getToggleState();
                manager.setPanelSettings (info.key, s);
            };

            addAndMakeVisible (exclusiveToggle);
            exclusiveToggle.setButtonText (LOC ("touchScreens.exclusive"));
            exclusiveToggle.setToggleState (p.settings.seize, juce::dontSendNotification);
            exclusiveToggle.onClick = [this]
            {
                auto s = info.settings;
                s.seize = exclusiveToggle.getToggleState();
                manager.setPanelSettings (info.key, s);
            };
        }

        juce::String statusText() const
        {
            if (info.problem.isNotEmpty())
                return info.problem;
            if (! manager.hasDisplay (info.settings))
                return LOC ("touchScreens.noDisplay");
            return LOC ("touchScreens.reading") + (info.seized ? ", " + LOC ("touchScreens.exclusive").toLowerCase() : juce::String());
        }

        void resized() override
        {
            auto r = getLocalBounds();
            auto line1 = r.removeFromTop (26);
            name.setBounds (line1.removeFromLeft (line1.getWidth() / 2));
            status.setBounds (line1);
            r.removeFromTop (6);
            auto line2 = r.removeFromTop (28);
            displayLabel.setBounds (line2.removeFromLeft (90));
            displayBox.setBounds (line2.removeFromLeft (juce::jmin (380, line2.getWidth())));
            r.removeFromTop (8);
            auto line3 = r.removeFromTop (26);
            cursorToggle.setBounds (line3.removeFromLeft (line3.getWidth() / 2));
            exclusiveToggle.setBounds (line3);
        }

        Manager& manager;
        Manager::PanelInfo info;
        std::vector<Manager::DisplayInfo> displays;
        juce::Label name, status, displayLabel;
        juce::ComboBox displayBox;
        juce::ToggleButton cursorToggle, exclusiveToggle;
    };

    void handleAsyncUpdate() override   { rebuild(); }

    void rebuild()
    {
        rows.clear();
        for (auto& p : manager.getPanels())
            if (p.connected)
                addAndMakeVisible (*rows.emplace_back (std::make_unique<Row> (manager, p)));
        resized();
        repaint();
        if (onHeightChanged)
            onHeightChanged (getIdealHeight());
    }

public:
    std::function<void (int)> onHeightChanged;

private:
    Manager& manager;
    std::function<void()> onIdentify;
    juce::TextButton identifyButton;
    juce::Label hint;
    std::vector<std::unique_ptr<Row>> rows;
    int listenerToken = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacTouchPanelsContent)
};

//==============================================================================
/** System Config > Touch Screens: the connected panels and their displays.
    Owned by MainComponent; hides on close. */
class MacTouchPanelsWindow : public juce::DocumentWindow
{
public:
    MacTouchPanelsWindow (spatcore::controllers::MacHidTouchManager& manager, std::function<void()> identify)
        : DocumentWindow (LOC ("touchScreens.windowTitle"), ColorScheme::get().background, DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar (true);
        auto* content = new MacTouchPanelsContent (manager, std::move (identify));
        content->onHeightChanged = [this] (int h) { setContentComponentSize (620, h); };
        content->setSize (620, content->getIdealHeight());
        setContentOwned (content, true);
        setResizable (false, false);
        centreWithSize (getWidth(), getHeight());
        WindowUtils::enableDarkTitleBar (this);
        WindowUtils::enableRawMultiTouch (this);
    }

    void closeButtonPressed() override   { setVisible (false); }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacTouchPanelsWindow)
};

#endif
