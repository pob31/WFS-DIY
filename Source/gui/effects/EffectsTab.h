#pragma once

#include <JuceHeader.h>
#include "EffectsTabContext.h"
#include "../ChannelSelector.h"
#include "../ColorScheme.h"
#include "../WfsLookAndFeel.h"
#include "../HelpCard.h"
#include "../StatusBar.h"
#include "../buttons/LongPressButton.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../../Parameters/WFSParameterDefaults.h"
#include "../../Localization/LocalizationManager.h"

/**
    The Effects tab: header, four sub-tabs, footer.

    DELIBERATELY NOT A MONOLITH. ReverbTab is 5859 lines in one class; this one
    keeps the header, the sub-tab switch and the configuration footer, and
    hands each sub-tab to its own panel. What the panels share - the reload
    guards, the write funnel, the hover-text maps - is EffectsTabContext, so
    there is exactly one copy of each.

    THE HEADER IS THE PART THAT IS NOT LIKE REVERB. An effects channel carries
    a link group AND a per-channel link mode (R5-5), a group-mute action rather
    than a coupling (R5-2), an emergency Clear, and two read-only indicators
    the engine drives: the loop guard, which mutes a channel's effect-to-effect
    feed when it runs away, and the feedback-cycle badge, which says this
    channel sits in an A -> B -> A loop the operator built on purpose.
*/
class EffectsTab : public juce::Component,
                   private juce::TextEditor::Listener,
                   private juce::ChangeListener,
                   private juce::ValueTree::Listener,
                   private juce::KeyListener,
                   public ColorScheme::Manager::Listener,
                   public HelpCardProvider
{
public:
    //==========================================================================
    /** A small round indicator the engine drives. Off by default, and it draws
        nothing at all when off - an LED that is always visible reads as a
        control the operator can press. */
    class StateLed : public juce::Component
    {
    public:
        StateLed (juce::Colour onColour) : colour (onColour) {}

        void setActive (bool shouldBeActive)
        {
            if (active == shouldBeActive)
                return;

            active = shouldBeActive;
            repaint();
        }

        bool isActive() const noexcept { return active; }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (2.0f);
            const float d = juce::jmin (r.getWidth(), r.getHeight());
            auto dot = juce::Rectangle<float> (d, d).withCentre (r.getCentre());

            g.setColour (active ? colour : ColorScheme::get().textDisabled.withAlpha (0.25f));
            g.fillEllipse (dot);

            if (active)
            {
                g.setColour (colour.brighter (0.6f));
                g.drawEllipse (dot, 1.0f);
            }
        }

    private:
        juce::Colour colour;
        bool active = false;
    };

    //==========================================================================
    explicit EffectsTab (WfsParameters& params)
        : ctx (params),
          effectsTree (params.getEffectTree()),
          configTree (params.getConfigTree()),
          ioTree (params.getConfigTree().getChildWithName (WFSParameterIDs::IO))
    {
        setWantsKeyboardFocus (true);
        setFocusContainerType (FocusContainerType::keyboardFocusContainer);

        if (effectsTree.isValid())
            effectsTree.addListener (this);
        configTree.addListener (this);
        if (ioTree.isValid())
            ioTree.addListener (this);
        ColorScheme::Manager::getInstance().addListener (this);

        setupHeader();
        setupSubTabs();
        setupFooter();
        setupHelpText();
        setupMouseListeners();

        noChannelsLabel.setText (LOC ("effects.noChannels"), juce::dontSendNotification);
        noChannelsLabel.setJustificationType (juce::Justification::centred);
        noChannelsLabel.setColour (juce::Label::textColourId, ColorScheme::get().textSecondary);
        addChildComponent (noChannelsLabel);

        const int numEffects = params.getNumEffectChannels();
        channelSelector.setNumChannels (numEffects > 0 ? numEffects : 1);

        if (numEffects > 0)
            loadChannelParameters (1);

        updateVisibility();
    }

    ~EffectsTab() override
    {
        if (effectsTree.isValid())
            effectsTree.removeListener (this);
        configTree.removeListener (this);
        if (ioTree.isValid())
            ioTree.removeListener (this);
        ColorScheme::Manager::getInstance().removeListener (this);
    }

    //==========================================================================
    // What MainComponent wires
    //==========================================================================

    std::function<void (int channel)> onChannelSelected;
    std::function<void (bool enabled)> onMapEditChanged;
    std::function<void (bool active)> onSoloEffectsChanged;

    /** fx = 0-based channel, or -1 for every channel. */
    std::function<void (int fx)> onClearRequested;

    std::function<void()> onConfigReloaded;
    std::function<void (int subTabIndex)> onSubTabChanged;

    void setStatusBar (StatusBar* bar) { ctx.statusBar = bar; }

    int getCurrentChannel() const { return ctx.currentChannel; }
    int getCurrentSubTab() const { return subTabBar.getCurrentTabIndex(); }

    void selectChannel (int channel)
    {
        const int numEffects = ctx.parameters.getNumEffectChannels();
        if (numEffects <= 0)
            return;

        const int clamped = juce::jlimit (1, numEffects, channel);
        channelSelector.setSelectedChannelProgrammatically (clamped);
        loadChannelParameters (clamped);
    }

    /** Stream Deck / OSC drives these. They move the GUI without firing the
        callbacks back out, which would loop. */
    void setSoloEffectsFromExternal (bool active)
    {
        if (soloEffectsActive == active)
            return;

        soloEffectsActive = active;
        refreshSoloEffectsButton();
    }

    void setEditOnMapFromExternal (bool enabled)
    {
        if (mapEditActive == enabled)
            return;

        mapEditActive = enabled;
        refreshMapEditButton();
    }

    /** Pushed at 50 Hz by MainComponent: what the engine knows and the tree
        does not. Everything here is read-only session state - the loop-guard
        trip, the feedback-cycle membership, and whether this channel is fed by
        anything at all, which is what makes it the ENTRY POINT of its bunch.
        The entry role is emergent (decision of 2026-09-22): no property names
        it, the sends grid already says who feeds whom. */
    void setLiveState (int fx, bool loopGuardTripped, bool inCycle, bool isEntryPoint)
    {
        if (fx != ctx.slot())
            return;

        loopGuardLed.setActive (loopGuardTripped);
        cycleLed.setActive (inCycle);
        entryLed.setActive (isEntryPoint);
    }

    /** After a project load or a config reload: the trees may have been
        replaced wholesale, so re-acquire them before reading anything. */
    void refreshFromValueTree()
    {
        if (effectsTree.isValid())
            effectsTree.removeListener (this);
        configTree.removeListener (this);
        if (ioTree.isValid())
            ioTree.removeListener (this);

        effectsTree = ctx.parameters.getEffectTree();
        configTree  = ctx.parameters.getConfigTree();
        ioTree      = configTree.getChildWithName (WFSParameterIDs::IO);

        if (effectsTree.isValid())
            effectsTree.addListener (this);
        configTree.addListener (this);
        if (ioTree.isValid())
            ioTree.addListener (this);

        const int numEffects = ctx.parameters.getNumEffectChannels();
        channelSelector.setNumChannels (numEffects > 0 ? numEffects : 1);
        ctx.currentChannel = juce::jlimit (1, juce::jmax (1, numEffects), ctx.currentChannel);

        if (numEffects > 0)
            loadChannelParameters (ctx.currentChannel);

        updateVisibility();
        resized();
        repaint();
    }

    //==========================================================================
    // Component
    //==========================================================================

    void paint (juce::Graphics& g) override
    {
        g.fillAll (ColorScheme::get().background);
    }

    void resized() override
    {
        layoutScale = static_cast<float> (getHeight()) / 932.0f;

        auto area = getLocalBounds().reduced (scaled (10));

        // The footer is laid out FIRST, as the reverb tab does, so Import stays
        // reachable when the session has no effects channels at all.
        layoutFooter (area.removeFromBottom (scaled (36)));
        area.removeFromBottom (scaled (6));

        layoutHeader (area.removeFromTop (scaled (34)));
        area.removeFromTop (scaled (6));

        subTabBar.setBounds (area.removeFromTop (scaled (28)));
        area.removeFromTop (scaled (6));

        subTabContentArea = area;
        noChannelsLabel.setBounds (area);
        layoutCurrentSubTab();
    }

    void colorSchemeChanged() override
    {
        noChannelsLabel.setColour (juce::Label::textColourId, ColorScheme::get().textSecondary);
        nameEditor.setColour (juce::TextEditor::backgroundColourId, ColorScheme::get().surfaceCard);
        nameEditor.setColour (juce::TextEditor::textColourId, ColorScheme::get().textPrimary);
        refreshMapEditButton();
        refreshSoloEffectsButton();
        repaint();
    }

    std::vector<HelpCardButton*> getVisibleHelpButtons() override
    {
        return {};   // help cards land with the panels
    }

private:
    //==========================================================================
    // Header
    //==========================================================================

    void setupHeader()
    {
        addAndMakeVisible (channelSelector);
        channelSelector.onChannelChanged = [this] (int channel)
        {
            loadChannelParameters (channel);
            if (onChannelSelected)
                onChannelSelected (channel);
        };

        addAndMakeVisible (nameEditor);
        nameEditor.addListener (this);
        nameEditor.addKeyListener (this);

        addAndMakeVisible (linkGroupCombo);
        linkGroupCombo.addItem (LOC ("effects.link.unlinked"), 1);
        for (int g = 1; g <= WFSParameterDefaults::effectLinkGroupMax; ++g)
            linkGroupCombo.addItem (groupName (g), g + 1);
        linkGroupCombo.onChange = [this]
        {
            ctx.write (WFSParameterIDs::effectLinkGroup, linkGroupCombo.getSelectedId() - 1);
            updateLinkModeEnabled();
        };

        addAndMakeVisible (linkModeCombo);
        linkModeCombo.addItem (LOC ("effects.link.modeOff"), 1);
        linkModeCombo.addItem (LOC ("effects.link.modeAbsolute"), 2);
        linkModeCombo.addItem (LOC ("effects.link.modeRelative"), 3);
        linkModeCombo.onChange = [this]
        {
            ctx.write (WFSParameterIDs::effectLinkMode, linkModeCombo.getSelectedId() - 1);
        };

        // A group mute WRITES every member once and leaves each one
        // independently editable afterwards (R5-2). It is not a link.
        addAndMakeVisible (groupMuteButton);
        groupMuteButton.setButtonText (LOC ("effects.buttons.groupMute"));
        groupMuteButton.onLongPress = [this]
        {
            const int group = ctx.readInt (WFSParameterIDs::effectLinkGroup, 0);
            if (group == 0)
            {
                ctx.showStatusMessage (LOC ("effects.messages.groupMuteNeedsGroup"));
                return;
            }

            groupMuted = ! groupMuted;
            {
                const juce::ScopedValueSetter<bool> selfWriteScope (ctx.isSelfWriting, true);
                ctx.parameters.getValueTreeState().setEffectGroupMute (group, groupMuted);
            }
            loadChannelParameters (ctx.currentChannel);
        };

        addAndMakeVisible (muteButton);
        muteButton.setButtonText (LOC ("effects.buttons.mute"));
        muteButton.setClickingTogglesState (true);
        muteButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFFFF5722));
        muteButton.onClick = [this]
        {
            ctx.write (WFSParameterIDs::effectMute, muteButton.getToggleState() ? 1 : 0);
        };

        addAndMakeVisible (soloButton);
        soloButton.setButtonText (LOC ("effects.buttons.solo"));
        soloButton.onLongPress = [this]
        {
            const bool soloed = ctx.readInt (WFSParameterIDs::effectSolo, 0) != 0;
            ctx.write (WFSParameterIDs::effectSolo, soloed ? 0 : 1);
            refreshSoloButton();
        };

        addAndMakeVisible (mapVisibilityButton);
        mapVisibilityButton.onClick = [this] { toggleMapVisibility(); };
        refreshMapVisibilityButton();

        addAndMakeVisible (mapEditButton);
        mapEditButton.onClick = [this]
        {
            mapEditActive = ! mapEditActive;

            // Editing on a Map that is not showing the returns is a gesture
            // with nothing to grab, so turning it on turns them on.
            if (mapEditActive && ! effectsVisibleOnMap())
                toggleMapVisibility();

            refreshMapEditButton();

            if (onMapEditChanged)
                onMapEditChanged (mapEditActive);
        };
        refreshMapEditButton();

        addAndMakeVisible (soloEffectsButton);
        soloEffectsButton.onLongPress = [this]
        {
            soloEffectsActive = ! soloEffectsActive;
            refreshSoloEffectsButton();

            if (onSoloEffectsChanged)
                onSoloEffectsChanged (soloEffectsActive);
        };
        refreshSoloEffectsButton();

        // Ctrl clears EVERY chain. A tail that will not stop is the case this
        // button exists for, and hunting for the right channel while it feeds
        // back is not a thing to ask of an operator.
        addAndMakeVisible (clearButton);
        clearButton.setButtonText (LOC ("effects.buttons.clear"));
        clearButton.setBaseColour (juce::Colour (0xFF8C3333));
        clearButton.onLongPress = [this]
        {
            const bool all = juce::ModifierKeys::getCurrentModifiersRealtime().isCommandDown()
                          || juce::ModifierKeys::getCurrentModifiersRealtime().isCtrlDown();

            if (onClearRequested)
                onClearRequested (all ? -1 : ctx.slot());

            ctx.showStatusMessage (all ? LOC ("effects.messages.clearedAll")
                                       : LOC ("effects.messages.cleared"));
        };

        addAndMakeVisible (loopGuardLed);
        addAndMakeVisible (cycleLed);
        addAndMakeVisible (entryLed);
    }

    void layoutHeader (juce::Rectangle<int> area)
    {
        const int spacing = scaled (5);

        channelSelector.setBounds (area.removeFromLeft (scaled (120)));
        area.removeFromLeft (spacing);
        nameEditor.setBounds (area.removeFromLeft (scaled (150)));
        area.removeFromLeft (spacing);

        // The three engine indicators travel together, left of the controls.
        const int led = scaled (16);
        loopGuardLed.setBounds (area.removeFromLeft (led).withSizeKeepingCentre (led, led));
        cycleLed.setBounds (area.removeFromLeft (led).withSizeKeepingCentre (led, led));
        entryLed.setBounds (area.removeFromLeft (led).withSizeKeepingCentre (led, led));
        area.removeFromLeft (spacing);

        linkGroupCombo.setBounds (area.removeFromLeft (scaled (110)));
        area.removeFromLeft (spacing);
        linkModeCombo.setBounds (area.removeFromLeft (scaled (100)));
        area.removeFromLeft (spacing);
        groupMuteButton.setBounds (area.removeFromLeft (scaled (95)));

        // Right-hand group, in the order the reverb header uses
        clearButton.setBounds (area.removeFromRight (scaled (80)));
        area.removeFromRight (spacing);
        soloEffectsButton.setBounds (area.removeFromRight (scaled (110)));
        area.removeFromRight (spacing);
        soloButton.setBounds (area.removeFromRight (scaled (70)));
        area.removeFromRight (spacing);
        muteButton.setBounds (area.removeFromRight (scaled (70)));
        area.removeFromRight (spacing);
        mapEditButton.setBounds (area.removeFromRight (scaled (100)));
        area.removeFromRight (spacing);
        mapVisibilityButton.setBounds (area.removeFromRight (scaled (100)));
    }

    //==========================================================================
    // Sub-tabs
    //==========================================================================

    void setupSubTabs()
    {
        addAndMakeVisible (subTabBar);
        const auto tabColour = juce::Colour (0xFF2A2A2A);
        subTabBar.addTab (LOC ("effects.tabs.channel"),  tabColour, -1);
        subTabBar.addTab (LOC ("effects.tabs.sends"),    tabColour, -1);
        subTabBar.addTab (LOC ("effects.tabs.chain"),    tabColour, -1);
        subTabBar.addTab (LOC ("effects.tabs.settings"), tabColour, -1);
        subTabBar.addChangeListener (static_cast<juce::ChangeListener*> (this));

        // The panels land in the commits that follow; until then the content
        // area names what will occupy it rather than sitting empty.
        addAndMakeVisible (placeholderLabel);
        placeholderLabel.setJustificationType (juce::Justification::centred);
        placeholderLabel.setColour (juce::Label::textColourId, ColorScheme::get().textDisabled);
    }

    void layoutCurrentSubTab()
    {
        placeholderLabel.setBounds (subTabContentArea);
        placeholderLabel.setText (subTabBar.getCurrentTabName(), juce::dontSendNotification);
    }

    void changeListenerCallback (juce::ChangeBroadcaster*) override
    {
        layoutCurrentSubTab();
        repaint();

        if (onSubTabChanged)
            onSubTabChanged (subTabBar.getCurrentTabIndex());
    }

    //==========================================================================
    // Footer
    //==========================================================================

    void setupFooter()
    {
        addAndMakeVisible (storeButton);
        storeButton.setButtonText (LOC ("effects.buttons.storeConfig"));
        storeButton.setBaseColour (juce::Colour (0xFF8C3333));
        storeButton.onLongPress = [this] { storeConfiguration(); };

        addAndMakeVisible (reloadButton);
        reloadButton.setButtonText (LOC ("effects.buttons.reloadConfig"));
        reloadButton.setBaseColour (juce::Colour (0xFF338C33));
        reloadButton.onLongPress = [this] { reloadConfiguration (false); };

        addAndMakeVisible (reloadBackupButton);
        reloadBackupButton.setButtonText (LOC ("effects.buttons.reloadBackup"));
        reloadBackupButton.setBaseColour (juce::Colour (0xFF266626));
        reloadBackupButton.onLongPress = [this] { reloadConfiguration (true); };

        addAndMakeVisible (importButton);
        importButton.setButtonText (LOC ("effects.buttons.import"));
        importButton.setBaseColour (juce::Colour (0xFF338C33));
        importButton.onLongPress = [this] { importConfiguration(); };

        addAndMakeVisible (exportButton);
        exportButton.setButtonText (LOC ("effects.buttons.export"));
        exportButton.setBaseColour (juce::Colour (0xFF8C3333));
        exportButton.onLongPress = [this] { exportConfiguration(); };
    }

    void layoutFooter (juce::Rectangle<int> area)
    {
        const int spacing = scaled (5);
        const int buttonWidth = (area.getWidth() - spacing * 4) / 5;

        storeButton.setBounds (area.removeFromLeft (buttonWidth));
        area.removeFromLeft (spacing);
        reloadButton.setBounds (area.removeFromLeft (buttonWidth));
        area.removeFromLeft (spacing);
        reloadBackupButton.setBounds (area.removeFromLeft (buttonWidth));
        area.removeFromLeft (spacing);
        importButton.setBounds (area.removeFromLeft (buttonWidth));
        area.removeFromLeft (spacing);
        exportButton.setBounds (area.removeFromLeft (buttonWidth));
    }

    void storeConfiguration()
    {
        auto& fileManager = ctx.parameters.getFileManager();
        if (! fileManager.hasValidProjectFolder())
        {
            ctx.showStatusMessage (LOC ("effects.messages.selectFolderFirst"));
            return;
        }

        if (fileManager.saveEffectsConfig())
            ctx.showStatusMessage (LOC ("effects.messages.configSaved"));
        else
            ctx.showStatusMessage (LOC ("effects.messages.error")
                                       .replace ("{error}", fileManager.getLastError()));
    }

    void reloadConfiguration (bool fromBackup)
    {
        auto& fileManager = ctx.parameters.getFileManager();
        if (! fileManager.hasValidProjectFolder())
        {
            ctx.showStatusMessage (LOC ("effects.messages.selectFolderFirst"));
            return;
        }

        const bool ok = fromBackup ? fileManager.loadEffectsConfigBackup()
                                   : fileManager.loadEffectsConfig();
        if (ok)
        {
            refreshFromValueTree();
            ctx.showStatusMessage (LOC ("effects.messages.configLoaded"));

            if (onConfigReloaded)
                onConfigReloaded();
        }
        else
        {
            ctx.showStatusMessage (LOC ("effects.messages.error")
                                       .replace ("{error}", fileManager.getLastError()));
        }
    }

    void importConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser> (
            LOC ("effects.dialogs.importTitle"), juce::File(), "*.xml");

        chooser->launchAsync (juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser] (const juce::FileChooser& fc)
            {
                const auto file = fc.getResult();
                if (file == juce::File())
                    return;

                if (ctx.parameters.getFileManager().importEffectsConfig (file))
                {
                    refreshFromValueTree();
                    ctx.showStatusMessage (LOC ("effects.messages.configLoaded"));

                    if (onConfigReloaded)
                        onConfigReloaded();
                }
                else
                {
                    ctx.showStatusMessage (LOC ("effects.messages.error")
                        .replace ("{error}", ctx.parameters.getFileManager().getLastError()));
                }
            });
    }

    void exportConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser> (
            LOC ("effects.dialogs.exportTitle"), juce::File(), "*.xml");

        chooser->launchAsync (juce::FileBrowserComponent::saveMode
                            | juce::FileBrowserComponent::canSelectFiles
                            | juce::FileBrowserComponent::warnAboutOverwriting,
            [this, chooser] (const juce::FileChooser& fc)
            {
                const auto file = fc.getResult();
                if (file == juce::File())
                    return;

                if (ctx.parameters.getFileManager().exportEffectsConfig (file))
                    ctx.showStatusMessage (LOC ("effects.messages.configSaved"));
                else
                    ctx.showStatusMessage (LOC ("effects.messages.error")
                        .replace ("{error}", ctx.parameters.getFileManager().getLastError()));
            });
    }

    //==========================================================================
    // Loading and visibility
    //==========================================================================

    void loadChannelParameters (int channel)
    {
        const int numEffects = ctx.parameters.getNumEffectChannels();
        if (numEffects <= 0)
            return;

        ctx.currentChannel = juce::jlimit (1, numEffects, channel);

        const juce::ScopedValueSetter<bool> loadingScope (ctx.isLoadingParameters, true);

        nameEditor.setText (ctx.read (WFSParameterIDs::effectName).toString(), false);

        const int group = juce::jlimit (0, WFSParameterDefaults::effectLinkGroupMax,
                                        ctx.readInt (WFSParameterIDs::effectLinkGroup, 0));
        linkGroupCombo.setSelectedId (group + 1, juce::dontSendNotification);

        const int mode = juce::jlimit (WFSParameterDefaults::effectLinkModeMin,
                                       WFSParameterDefaults::effectLinkModeMax,
                                       ctx.readInt (WFSParameterIDs::effectLinkMode,
                                                    WFSParameterDefaults::effectLinkModeDefault));
        linkModeCombo.setSelectedId (mode + 1, juce::dontSendNotification);
        updateLinkModeEnabled();

        muteButton.setToggleState (ctx.readInt (WFSParameterIDs::effectMute, 0) != 0,
                                   juce::dontSendNotification);
        refreshSoloButton();
    }

    void updateVisibility()
    {
        const bool has = ctx.hasChannels();

        noChannelsLabel.setVisible (! has);
        placeholderLabel.setVisible (has);
        subTabBar.setVisible (has);

        for (auto* c : { static_cast<juce::Component*> (&channelSelector),
                         static_cast<juce::Component*> (&nameEditor),
                         static_cast<juce::Component*> (&linkGroupCombo),
                         static_cast<juce::Component*> (&linkModeCombo),
                         static_cast<juce::Component*> (&groupMuteButton),
                         static_cast<juce::Component*> (&muteButton),
                         static_cast<juce::Component*> (&soloButton),
                         static_cast<juce::Component*> (&mapVisibilityButton),
                         static_cast<juce::Component*> (&mapEditButton),
                         static_cast<juce::Component*> (&soloEffectsButton),
                         static_cast<juce::Component*> (&clearButton),
                         static_cast<juce::Component*> (&loopGuardLed),
                         static_cast<juce::Component*> (&cycleLed),
                         static_cast<juce::Component*> (&entryLed) })
            c->setVisible (has);

        // With no channels the three session toggles have nothing to act on,
        // and leaving them lit would say the show is soloed when it is not.
        if (! has)
        {
            if (soloEffectsActive)
            {
                soloEffectsActive = false;
                refreshSoloEffectsButton();

                if (onSoloEffectsChanged)
                    onSoloEffectsChanged (false);
            }

            if (mapEditActive)
            {
                mapEditActive = false;
                refreshMapEditButton();

                if (onMapEditChanged)
                    onMapEditChanged (false);
            }

            groupMuted = false;
        }

        // The configuration footer stays reachable at zero channels: Import is
        // how a session gets its effects back.
    }

    void updateLinkModeEnabled()
    {
        // A mode with no group to act on is a control that does nothing, so it
        // greys out exactly as Apply-to-Array does for a Single output.
        linkModeCombo.setEnabled (linkGroupCombo.getSelectedId() > 1);
    }

    //==========================================================================
    // Map visibility and the header toggles
    //==========================================================================

    bool effectsVisibleOnMap() const
    {
        const auto v = ctx.parameters.getConfigParam ("effectsMapVisible");
        return v.isVoid() || static_cast<int> (v) != 0;
    }

    void toggleMapVisibility()
    {
        const bool next = ! effectsVisibleOnMap();
        {
            const juce::ScopedValueSetter<bool> selfWriteScope (ctx.isSelfWriting, true);
            ctx.parameters.setConfigParam ("effectsMapVisible", next ? 1 : 0);
        }
        refreshMapVisibilityButton();
    }

    void refreshMapVisibilityButton()
    {
        const bool visible = effectsVisibleOnMap();
        mapVisibilityButton.setButtonText (visible ? LOC ("effects.buttons.mapVisible")
                                                   : LOC ("effects.buttons.mapHidden"));
        mapVisibilityButton.setColour (juce::TextButton::buttonColourId,
                                       visible ? juce::Colour (0xFF3A6EA5)
                                               : getLookAndFeel().findColour (juce::TextButton::buttonColourId));
    }

    void refreshMapEditButton()
    {
        mapEditButton.setButtonText (mapEditActive ? LOC ("effects.buttons.editOnMapOn")
                                                   : LOC ("effects.buttons.editOnMap"));
        mapEditButton.setColour (juce::TextButton::buttonColourId,
                                 mapEditActive ? juce::Colour (0xFFCC8800)
                                               : getLookAndFeel().findColour (juce::TextButton::buttonColourId));
    }

    void refreshSoloEffectsButton()
    {
        soloEffectsButton.setButtonText (soloEffectsActive ? LOC ("effects.buttons.soloEffectsOn")
                                                           : LOC ("effects.buttons.soloEffects"));
        soloEffectsButton.setBaseColour (soloEffectsActive ? juce::Colour (0xFFCC8800) : juce::Colour());
    }

    void refreshSoloButton()
    {
        const bool soloed = ctx.readInt (WFSParameterIDs::effectSolo, 0) != 0;
        soloButton.setBaseColour (soloed ? juce::Colour (0xFFCC8800) : juce::Colour());
    }

    juce::String groupName (int group) const
    {
        const auto names = ctx.parameters.getConfigParam ("effectsGlobalLinkNames").toString();
        juce::StringArray tokens;
        tokens.addTokens (names, ",", "");

        const auto name = tokens[group - 1].trim();
        return name.isNotEmpty() ? name : LOC ("effects.link.group") + " " + juce::String (group);
    }

    //==========================================================================
    // Listeners
    //==========================================================================

    void textEditorReturnKeyPressed (juce::TextEditor& editor) override
    {
        if (&editor == &nameEditor)
            commitName();
    }

    void textEditorFocusLost (juce::TextEditor& editor) override
    {
        if (&editor == &nameEditor)
            commitName();
    }

    void commitName()
    {
        if (ctx.isLoadingParameters || ! ctx.hasChannels())
            return;

        ctx.write (WFSParameterIDs::effectName, nameEditor.getText());
    }

    bool keyPressed (const juce::KeyPress& key, juce::Component* origin) override
    {
        if (origin == &nameEditor && key == juce::KeyPress::escapeKey)
        {
            nameEditor.setText (ctx.read (WFSParameterIDs::effectName).toString(), false);
            unfocusAllComponents();
            return true;
        }

        return false;
    }

    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override
    {
        // 1. The channel count, from <IO>
        if (property == WFSParameterIDs::effectChannels)
        {
            const int numEffects = ctx.parameters.getNumEffectChannels();
            channelSelector.setNumChannels (numEffects > 0 ? numEffects : 1);
            ctx.currentChannel = juce::jlimit (1, juce::jmax (1, numEffects), ctx.currentChannel);

            if (numEffects > 0)
                loadChannelParameters (ctx.currentChannel);

            updateVisibility();
            resized();
            return;
        }

        // 2. The map toggle, wherever it is written from
        if (property == WFSParameterIDs::effectsMapVisible)
        {
            refreshMapVisibilityButton();
            return;
        }

        // 3. OUR OWN WRITES STOP HERE. Without this, every delta of a drag
        //    would schedule a full channel reload, which floods the message
        //    queue and fights the hand that is dragging.
        if (ctx.isSelfWriting || ctx.isLoadingParameters)
            return;

        // 4. Anything else under the SELECTED <Effect>: one coalesced reload.
        //    The walk is by node TYPE and counts Effect children, because that
        //    is how every effects accessor resolves a channel.
        auto node = tree;
        while (node.isValid() && ! node.hasType (WFSParameterIDs::Effect))
            node = node.getParent();

        if (! node.isValid())
            return;

        auto effects = node.getParent();
        int dense = 0;
        for (int i = 0; i < effects.getNumChildren(); ++i)
        {
            auto child = effects.getChild (i);
            if (! child.hasType (WFSParameterIDs::Effect))
                continue;

            if (child == node)
                break;

            ++dense;
        }

        if (dense != ctx.slot() || channelReloadPending)
            return;

        channelReloadPending = true;
        juce::MessageManager::callAsync ([this]
        {
            channelReloadPending = false;
            loadChannelParameters (ctx.currentChannel);
        });
    }

    //==========================================================================
    // Hover help
    //==========================================================================

    void setupHelpText()
    {
        ctx.helpTextMap[&channelSelector]     = LOC ("effects.help.channelSelector");
        ctx.helpTextMap[&nameEditor]          = LOC ("effects.help.name");
        ctx.helpTextMap[&linkGroupCombo]      = LOC ("effects.help.linkGroup");
        ctx.helpTextMap[&linkModeCombo]       = LOC ("effects.help.linkMode");
        ctx.helpTextMap[&groupMuteButton]     = LOC ("effects.help.groupMute");
        ctx.helpTextMap[&muteButton]          = LOC ("effects.help.mute");
        ctx.helpTextMap[&soloButton]          = LOC ("effects.help.solo");
        ctx.helpTextMap[&mapVisibilityButton] = LOC ("effects.help.mapVisible");
        ctx.helpTextMap[&mapEditButton]       = LOC ("effects.help.editOnMap");
        ctx.helpTextMap[&soloEffectsButton]   = LOC ("effects.help.soloEffects");
        ctx.helpTextMap[&clearButton]         = LOC ("effects.help.clear");
        ctx.helpTextMap[&loopGuardLed]        = LOC ("effects.help.loopGuard");
        ctx.helpTextMap[&cycleLed]            = LOC ("effects.help.cycle");
        ctx.helpTextMap[&entryLed]            = LOC ("effects.help.entry");
        ctx.helpTextMap[&storeButton]         = LOC ("effects.help.storeConfig");
        ctx.helpTextMap[&reloadButton]        = LOC ("effects.help.reloadConfig");
        ctx.helpTextMap[&reloadBackupButton]  = LOC ("effects.help.reloadBackup");
        ctx.helpTextMap[&importButton]        = LOC ("effects.help.import");
        ctx.helpTextMap[&exportButton]        = LOC ("effects.help.export");
    }

    void setupMouseListeners()
    {
        for (auto& entry : ctx.helpTextMap)
            entry.first->addMouseListener (this, true);
    }

    void mouseEnter (const juce::MouseEvent& e) override
    {
        if (ctx.statusBar == nullptr)
            return;

        // Walk up: a ComboBox hands us its internal child, not itself.
        for (auto* c = e.eventComponent; c != nullptr && c != this; c = c->getParentComponent())
        {
            const auto it = ctx.helpTextMap.find (c);
            if (it != ctx.helpTextMap.end())
            {
                ctx.statusBar->setHelpText (it->second);
                return;
            }
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (ctx.statusBar != nullptr)
            ctx.statusBar->clearText();
    }

    //==========================================================================
    int scaled (int ref) const
    {
        return juce::jmax (static_cast<int> (ref * 0.65f), static_cast<int> (ref * layoutScale));
    }

    EffectsTabContext ctx;

    juce::ValueTree effectsTree;
    juce::ValueTree configTree;
    juce::ValueTree ioTree;

    float layoutScale = 1.0f;
    juce::Rectangle<int> subTabContentArea;
    bool channelReloadPending = false;

    // Session-only header state. None of it is a parameter: solo and edit-on-map
    // are monitoring, and the group-mute latch is only which way the button
    // will act next.
    bool soloEffectsActive = false;
    bool mapEditActive = false;
    bool groupMuted = false;

    // Header
    ChannelSelectorButton channelSelector { "Effect" };
    juce::TextEditor nameEditor;
    juce::ComboBox linkGroupCombo;
    juce::ComboBox linkModeCombo;
    LongPressButton groupMuteButton { 800 };
    juce::TextButton muteButton;
    LongPressButton soloButton { 800 };
    juce::TextButton mapVisibilityButton;
    juce::TextButton mapEditButton;
    LongPressButton soloEffectsButton { 800 };
    LongPressButton clearButton { 800 };
    StateLed loopGuardLed { juce::Colour (0xFFFF5722) };
    StateLed cycleLed     { juce::Colour (0xFFD4A017) };
    StateLed entryLed     { juce::Colour (0xFF4CAF50) };

    // Sub-tabs
    juce::TabbedButtonBar subTabBar { juce::TabbedButtonBar::TabsAtTop };
    juce::Label placeholderLabel;
    juce::Label noChannelsLabel;

    // Footer
    LongPressButton storeButton { 1000 };
    LongPressButton reloadButton { 1000 };
    LongPressButton reloadBackupButton { 1000 };
    LongPressButton importButton { 1000 };
    LongPressButton exportButton { 1000 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectsTab)
};
