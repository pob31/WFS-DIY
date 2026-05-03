#pragma once

#include <JuceHeader.h>

#include "../Controllers/Touch/TouchManager.h"
#include "../Localization/LocalizationManager.h"
#include "ColorScheme.h"

#if defined (__linux__)

/**
    Linux-only settings dialog for mapping connected touchscreens to JUCE
    displays. One row per detected device, with a display picker and
    swap/flip toggles. Auto-refreshes on hotplug via the manager's change
    callback.

    Visible only on Linux when at least one touchscreen is detected.
*/
class LinuxTouchscreenPanel : public juce::Component
{
public:
    explicit LinuxTouchscreenPanel (WFSTouch::EvdevTouchManager& mgr)
        : manager (mgr)
    {
        listenerToken = manager.addChangeListener ([this] { rebuild(); });
        rebuild();
    }

    ~LinuxTouchscreenPanel() override
    {
        manager.removeChangeListener (listenerToken);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (ColorScheme::get().background);

        if (rows.isEmpty())
        {
            g.setColour (ColorScheme::get().textSecondary);
            g.setFont (juce::FontOptions (14.0f));
            g.drawFittedText (LOC ("touchscreens.empty"),
                              getLocalBounds().reduced (16),
                              juce::Justification::centred, 4);
        }
    }

    void resized() override
    {
        constexpr int rowH    = 64;
        constexpr int margin  = 12;
        auto area = getLocalBounds().reduced (margin);

        for (auto* row : rows)
        {
            row->setBounds (area.removeFromTop (rowH));
            area.removeFromTop (8);
        }
    }

    int getIdealHeight() const noexcept
    {
        constexpr int rowH = 64;
        constexpr int margin = 12;
        if (rows.isEmpty()) return 200;
        return margin * 2 + (int) rows.size() * (rowH + 8);
    }

private:
    //==========================================================================
    class Row : public juce::Component
    {
    public:
        Row (WFSTouch::EvdevTouchManager& mgr,
             WFSTouch::EvdevTouchManager::DeviceInfo info)
            : manager (mgr), device (std::move (info))
        {
            nameLabel.setText (device.displayName, juce::dontSendNotification);
            nameLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
            addAndMakeVisible (nameLabel);

            statusLabel.setFont (juce::FontOptions (11.0f));
            statusLabel.setColour (juce::Label::textColourId, ColorScheme::get().textSecondary);
            addAndMakeVisible (statusLabel);

            displayCombo.addItem (LOC ("touchscreens.disabled"), 1);
            auto& displays = juce::Desktop::getInstance().getDisplays().displays;
            for (int i = 0; i < displays.size(); ++i)
            {
                const auto& d = displays.getReference (i);
                juce::String label;
                label << "Display " << (i + 1)
                      << "  (" << d.userArea.getWidth() << "×" << d.userArea.getHeight() << ")";
                if (d.isMain) label << "  [primary]";
                displayCombo.addItem (label, i + 2);
            }
            displayCombo.setSelectedId (device.mapping.displayIndex < 0
                                            ? 1
                                            : device.mapping.displayIndex + 2,
                                        juce::dontSendNotification);
            displayCombo.onChange = [this] { onMappingChanged(); };
            addAndMakeVisible (displayCombo);

            swapToggle.setButtonText (LOC ("touchscreens.swapXY"));
            swapToggle.setToggleState (device.mapping.swapXY, juce::dontSendNotification);
            swapToggle.onClick = [this] { onMappingChanged(); };
            addAndMakeVisible (swapToggle);

            flipXToggle.setButtonText (LOC ("touchscreens.flipX"));
            flipXToggle.setToggleState (device.mapping.flipX, juce::dontSendNotification);
            flipXToggle.onClick = [this] { onMappingChanged(); };
            addAndMakeVisible (flipXToggle);

            flipYToggle.setButtonText (LOC ("touchscreens.flipY"));
            flipYToggle.setToggleState (device.mapping.flipY, juce::dontSendNotification);
            flipYToggle.onClick = [this] { onMappingChanged(); };
            addAndMakeVisible (flipYToggle);

            updateStatus();
        }

        void updateInfo (const WFSTouch::EvdevTouchManager::DeviceInfo& info)
        {
            device = info;
            updateStatus();
        }

        const juce::String& getSysPath() const noexcept { return device.sysPath; }

        void resized() override
        {
            auto r = getLocalBounds();
            auto top    = r.removeFromTop (24);
            auto bottom = r;

            nameLabel  .setBounds (top.removeFromLeft (240));
            statusLabel.setBounds (top);

            displayCombo.setBounds (bottom.removeFromLeft (240).reduced (0, 2));
            bottom.removeFromLeft (8);
            const int toggleW = (bottom.getWidth() - 16) / 3;
            swapToggle .setBounds (bottom.removeFromLeft (toggleW));
            bottom.removeFromLeft (8);
            flipXToggle.setBounds (bottom.removeFromLeft (toggleW));
            bottom.removeFromLeft (8);
            flipYToggle.setBounds (bottom);
        }

    private:
        void onMappingChanged()
        {
            const int sel = displayCombo.getSelectedId();
            device.mapping.displayIndex = (sel <= 1) ? -1 : (sel - 2);
            device.mapping.swapXY = swapToggle.getToggleState();
            device.mapping.flipX  = flipXToggle.getToggleState();
            device.mapping.flipY  = flipYToggle.getToggleState();
            device.mapping.sysPath = device.sysPath;
            manager.setMapping (device.sysPath, device.mapping);
            // The manager will fire its change callback which triggers a
            // rebuild and applies the new state; updateStatus runs on next
            // refresh.
        }

        void updateStatus()
        {
            juce::String s;
            if (device.hasError)
                s = LOC ("touchscreens.errorPrefix") + " " + device.errorMessage;
            else if (device.mapping.isEnabled() && device.isOpen)
                s = LOC ("touchscreens.statusActive");
            else if (device.mapping.isEnabled())
                s = LOC ("touchscreens.statusOpening");
            else
                s = LOC ("touchscreens.statusInactive");
            statusLabel.setText (s, juce::dontSendNotification);
        }

        WFSTouch::EvdevTouchManager& manager;
        WFSTouch::EvdevTouchManager::DeviceInfo device;

        juce::Label       nameLabel, statusLabel;
        juce::ComboBox    displayCombo;
        juce::ToggleButton swapToggle, flipXToggle, flipYToggle;
    };

    void rebuild()
    {
        auto devices = manager.getDetectedDevices();

        // Remove rows whose device is gone.
        for (int i = rows.size() - 1; i >= 0; --i)
        {
            bool stillThere = false;
            for (auto& d : devices) if (d.sysPath == rows[i]->getSysPath()) { stillThere = true; break; }
            if (! stillThere) rows.remove (i);
        }

        // Add or refresh rows in device order.
        for (auto& d : devices)
        {
            Row* found = nullptr;
            for (auto* r : rows) if (r->getSysPath() == d.sysPath) { found = r; break; }
            if (found != nullptr)
            {
                found->updateInfo (d);
            }
            else
            {
                auto* r = new Row (manager, d);
                addAndMakeVisible (r);
                rows.add (r);
            }
        }

        resized();
        repaint();
    }

    WFSTouch::EvdevTouchManager& manager;
    int listenerToken = 0;
    juce::OwnedArray<Row> rows;
};

//==============================================================================
class LinuxTouchscreenWindow : public juce::DocumentWindow
{
public:
    explicit LinuxTouchscreenWindow (WFSTouch::EvdevTouchManager& mgr)
        : juce::DocumentWindow (LOC ("touchscreens.title"),
                                ColorScheme::get().background,
                                juce::DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar (true);

        auto* panel = new LinuxTouchscreenPanel (mgr);
        setContentOwned (panel, false);

        const int idealH = juce::jmax (200, panel->getIdealHeight());
        setSize (760, idealH);
        setResizable (true, false);

        centreWithSize (getWidth(), getHeight());
        setVisible (true);
    }

    void closeButtonPressed() override
    {
        // Hide only; the parent (MainComponent) owns the unique_ptr and is
        // responsible for any deletion. Calling reset() from within this
        // method is unsafe — it would free `this` mid-callback.
        setVisible (false);
    }
};

#endif // __linux__
