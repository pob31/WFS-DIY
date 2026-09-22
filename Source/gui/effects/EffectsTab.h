#pragma once

#include <JuceHeader.h>
#include "EffectsTabContext.h"
#include "EffectsChannelPanel.h"
#include "EffectsMovementsPanel.h"
#include "EffectsSendsPanel.h"
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
    /** Sub-tab order, which several sites test against. */
    struct SubTab
    {
        static constexpr int Channel   = 0;
        static constexpr int Chain     = 1;
        static constexpr int Sends     = 2;   // shown as "Post-Processing": where the output goes
        static constexpr int Movements = 3;
        static constexpr int Settings  = 4;
    };

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

    void setOtomoProcessor (AutomOtionProcessor* p) { movementsPanel.setOtomoProcessor (p); }

    /** The output count moved: the mute grid and the array trims follow it. */
    void refreshOutputDependentControls() { channelPanel.refreshOutputDependentControls(); }

    void updateOtomoLevelIndicators (float shortPeakDb, float rmsDb)
    {
        movementsPanel.updateLevelIndicators (shortPeakDb, rmsDb);
    }

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
    /** The calculation engine's feedback-cycle bitmask, one bit per effect:
        the sends grid badges every column in a cycle, not just the selected one. */
    void setCycleMask (juce::uint32 mask) { sendsPanel.setCycleMask (mask); }

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
        else
            sendsPanel.refresh();

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

        // Header and footer bands with their divider lines: ReverbTab::paint
        g.setColour (ColorScheme::get().chromeSurface);
        g.fillRect (0, 0, getWidth(), headerHeight);
        g.fillRect (0, getHeight() - footerHeight, getWidth(), footerHeight);

        g.setColour (ColorScheme::get().chromeDivider);
        g.drawLine (0.0f, static_cast<float> (headerHeight),
                    static_cast<float> (getWidth()), static_cast<float> (headerHeight), 1.0f);
        g.drawLine (0.0f, static_cast<float> (getHeight() - footerHeight),
                    static_cast<float> (getWidth()), static_cast<float> (getHeight() - footerHeight), 1.0f);
    }

    void resized() override
    {
        layoutScale = static_cast<float> (getHeight()) / 932.0f;
        headerHeight = scaled (60);
        footerHeight = scaled (30) + 2 * scaled (10);

        // The footer is laid out FIRST, as the reverb tab does, so Import stays
        // reachable when the session has no effects channels at all.
        layoutFooter (getLocalBounds().removeFromBottom (footerHeight).reduced (scaled (10)));
        layoutHeader (getLocalBounds().removeFromTop (headerHeight).reduced (scaled (10), scaled (15)));

        auto area = getLocalBounds().withTrimmedTop (headerHeight).withTrimmedBottom (footerHeight)
                                    .reduced (scaled (10), scaled (6));

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
        refreshMuteButton();
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

        addAndMakeVisible (nameLabel);
        nameLabel.setText (LOC ("effects.labels.name"), juce::dontSendNotification);

        addAndMakeVisible (nameEditor);
        nameEditor.addListener (this);
        nameEditor.addKeyListener (this);

        // The link-group controls live on the Channel Parameters tab's top
        // row, where the Reverb tab keeps its own cross-channel control.

        // Per-channel mute: an instant toggle, worded and coloured like the
        // reverb tab's Mute Pre / Mute Post so the two headers read alike.
        addAndMakeVisible (muteButton);
        muteButton.setButtonText (LOC ("effects.buttons.mute"));
        muteButton.onClick = [this]
        {
            const bool next = ! (ctx.readInt (WFSParameterIDs::effectMute, 0) != 0);
            ctx.write (WFSParameterIDs::effectMute, next ? 1 : 0);
            refreshMuteButton();
        };

        addAndMakeVisible (soloButton);
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
        // ReverbTab::layoutHeader, width for width: selector, Name, the two
        // map buttons on the left; the long-press group on the right.
        const int rowHeight = scaled (30);
        const int spacing = scaled (5);

        auto row = area.removeFromTop (rowHeight);

        channelSelector.setBounds (row.removeFromLeft (scaled (150)));
        row.removeFromLeft (spacing * 2);

        nameLabel.setBounds (row.removeFromLeft (scaled (50)));
        nameEditor.setBounds (row.removeFromLeft (scaled (200)));

        row.removeFromLeft (spacing * 4);
        mapVisibilityButton.setBounds (row.removeFromLeft (scaled (180)));
        row.removeFromLeft (spacing);
        mapEditButton.setBounds (row.removeFromLeft (scaled (140)));

        // The three engine indicators, after the map buttons
        row.removeFromLeft (spacing * 3);
        const int led = scaled (16);
        loopGuardLed.setBounds (row.removeFromLeft (led).withSizeKeepingCentre (led, led));
        cycleLed.setBounds (row.removeFromLeft (led).withSizeKeepingCentre (led, led));
        entryLed.setBounds (row.removeFromLeft (led).withSizeKeepingCentre (led, led));

        // Right-aligned group, as the reverb header: Solo Effects, Mute, Solo, Clear
        clearButton.setBounds (row.removeFromRight (scaled (110)));
        row.removeFromRight (spacing);
        soloButton.setBounds (row.removeFromRight (scaled (110)));
        row.removeFromRight (spacing);
        muteButton.setBounds (row.removeFromRight (scaled (110)));
        row.removeFromRight (spacing);
        soloEffectsButton.setBounds (row.removeFromRight (scaled (130)));
    }

    //==========================================================================
    // Sub-tabs
    //==========================================================================

    void setupSubTabs()
    {
        addAndMakeVisible (subTabBar);
        const auto tabColour = juce::Colour (0xFF2A2A2A);
        // THE ORDER IS THE SIGNAL PATH, not the order these were written
        // (user, 2026-09-22): where the channel sits, then what it does to the
        // sound, then who feeds it, then how it travels, then the globals.
        subTabBar.addTab (LOC ("effects.tabs.channelParams"),  tabColour, -1);   // SubTab::Channel
        subTabBar.addTab (LOC ("effects.tabs.chain"),          tabColour, -1);   // SubTab::Chain
        subTabBar.addTab (LOC ("effects.tabs.postProcessing"), tabColour, -1);   // SubTab::Sends - the matrix
        subTabBar.addTab (LOC ("effects.tabs.movements"), tabColour, -1);   // SubTab::Movements
        subTabBar.addTab (LOC ("effects.tabs.settings"),  tabColour, -1);   // SubTab::Settings
        subTabBar.addChangeListener (static_cast<juce::ChangeListener*> (this));

        addChildComponent (channelPanel);
        addChildComponent (movementsPanel);
        addChildComponent (sendsPanel);

        // The remaining panels land in the commits that follow; until then the
        // content area names what will occupy it rather than sitting empty.
        addChildComponent (placeholderLabel);
        placeholderLabel.setJustificationType (juce::Justification::centred);
        placeholderLabel.setColour (juce::Label::textColourId, ColorScheme::get().textDisabled);
    }

    void layoutCurrentSubTab()
    {
        const int index = subTabBar.getCurrentTabIndex();
        const bool has = ctx.hasChannels();

        channelPanel.setVisible (has && index == SubTab::Channel);
        channelPanel.setBounds (subTabContentArea);

        movementsPanel.setVisible (has && index == SubTab::Movements);
        movementsPanel.setBounds (subTabContentArea);

        sendsPanel.setVisible (has && index == SubTab::Sends);
        sendsPanel.setBounds (subTabContentArea);

        const bool built = index == SubTab::Channel || index == SubTab::Movements || index == SubTab::Sends;
        placeholderLabel.setVisible (has && ! built);
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

        refreshMuteButton();
        refreshSoloButton();

        channelPanel.loadParameters();
        movementsPanel.loadParameters();
        sendsPanel.refresh();
    }

    void updateVisibility()
    {
        const bool has = ctx.hasChannels();

        noChannelsLabel.setVisible (! has);
        subTabBar.setVisible (has);
        layoutCurrentSubTab();

        for (auto* c : { static_cast<juce::Component*> (&channelSelector),
                         static_cast<juce::Component*> (&nameLabel),
                         static_cast<juce::Component*> (&nameEditor),
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

        }

        // The configuration footer stays reachable at zero channels: Import is
        // how a session gets its effects back.
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
        // Text-only state, as the reverb tab's "Reverbs Visible on Map"
        const bool visible = effectsVisibleOnMap();
        mapVisibilityButton.setButtonText (visible ? LOC ("effects.buttons.visibleOnMap")
                                                   : LOC ("effects.buttons.hiddenOnMap"));
    }

    void refreshMuteButton()
    {
        const bool muted = ctx.readInt (WFSParameterIDs::effectMute, 0) != 0;
        muteButton.setButtonText (muted ? LOC ("effects.buttons.muteOn") : LOC ("effects.buttons.mute"));
        muteButton.setColour (juce::TextButton::buttonColourId,
                              muted ? juce::Colour (0xFFCC8800)
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
        soloButton.setButtonText (soloed ? LOC ("effects.buttons.soloOn") : LOC ("effects.buttons.solo"));
        soloButton.setBaseColour (soloed ? juce::Colour (0xFFCC8800) : juce::Colour());
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
    int headerHeight = 60;
    int footerHeight = 50;
    juce::Rectangle<int> subTabContentArea;
    bool channelReloadPending = false;

    // Session-only header state. None of it is a parameter: solo and edit-on-map
    // are monitoring, and the group-mute latch is only which way the button
    // will act next.
    bool soloEffectsActive = false;
    bool mapEditActive = false;

    // Header
    ChannelSelectorButton channelSelector { "Effect" };
    juce::Label nameLabel;
    juce::TextEditor nameEditor;
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
    EffectsChannelPanel channelPanel { ctx };
    EffectsMovementsPanel movementsPanel { ctx };
    EffectsSendsPanel sendsPanel { ctx };
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
