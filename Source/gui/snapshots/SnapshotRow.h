#pragma once

#include <JuceHeader.h>
#include <map>
#include "SnapshotSession.h"
#include "../StatusBar.h"
#include "../RefreshableComboBox.h"
#include "../buttons/LongPressButton.h"
#include "../../Accessibility/TTSManager.h"
#include "../../Localization/LocalizationManager.h"

//==============================================================================
/**
    The snapshot row: selector, Store, Reload, Reload w/o Scope, Update, Edit
    Scope, Delete - the Inputs tab's footer row, now a component, so the Effects
    tab can show the same row over the same SnapshotSession (plan revision 8: one
    snapshot file carries both families).

    The only difference between the two instances is `scopeFamily`: the grid Edit
    Scope opens the Scope window on. Everything else - the selection, the session
    scope, the window - is the session's, so the two rows always agree.

    Layout is the Inputs tab's footer row verbatim: 7 buttons + a 1.5x selector
    over 8 units, Delete taking the remainder. The host passes its own spacing so
    the row lands pixel for pixel where it used to.

    Hover help is the row's own (the hosts keep private help maps): the status
    bar line and the TTS announcement the Inputs tab used to give these controls.
*/
class SnapshotRow : public juce::Component,
                    private SnapshotSession::Listener
{
public:
    using Family = WFSFileManager::SnapshotFamily;

    SnapshotRow (SnapshotSession& s, Family family)
        : session (s), scopeFamily (family)
    {
        addAndMakeVisible (storeButton);
        storeButton.setButtonText (LOC ("inputs.buttons.storeSnapshot"));
        storeButton.setBaseColour (juce::Colour (0xFF996633));  // Yellow-orange
        storeButton.onLongPress = [this] { session.store (this); };

        addAndMakeVisible (selector);
        selector.onChange = [this]
        {
            if (rebuilding)
                return;
            session.select (selector.getSelectedId() > 1 ? selector.getText() : juce::String());
        };
        selector.onPopupAboutToShow = [this] { session.refreshList(); };

        addAndMakeVisible (reloadButton);
        reloadButton.setButtonText (LOC ("inputs.buttons.reloadSnapshot"));
        reloadButton.setBaseColour (juce::Colour (0xFF669933));  // Yellow-green
        reloadButton.onLongPress = [this] { session.reload (this); };

        addAndMakeVisible (reloadWithoutScopeButton);
        reloadWithoutScopeButton.setButtonText (LOC ("inputs.buttons.reloadWithoutScope"));
        reloadWithoutScopeButton.setBaseColour (juce::Colour (0xFF669933));  // Yellow-green
        reloadWithoutScopeButton.onLongPress = [this] { session.reloadWithoutScope (this); };

        addAndMakeVisible (updateButton);
        updateButton.setButtonText (LOC ("inputs.buttons.updateSnapshot"));
        updateButton.setBaseColour (juce::Colour (0xFF996633));  // Yellow-orange
        updateButton.onLongPress = [this] { session.update(); };

        addAndMakeVisible (editScopeButton);
        editScopeButton.setButtonText (LOC ("inputs.buttons.editScope"));
        editScopeButton.setBaseColour (juce::Colour (0xFF33668C));  // Light blue
        editScopeButton.onLongPress = [this] { session.editScope (scopeFamily); };

        addAndMakeVisible (deleteButton);
        deleteButton.setButtonText (LOC ("inputs.buttons.deleteSnapshot"));
        deleteButton.setBaseColour (juce::Colour (0xFF661A33));  // Burgundy
        deleteButton.onLongPress = [this] { session.remove(); };

        helpText[&storeButton]              = LOC ("inputs.help.storeSnapshot");
        helpText[&selector]                 = LOC ("inputs.help.snapshotSelector");
        helpText[&reloadButton]             = LOC ("inputs.help.reloadSnapshot");
        helpText[&reloadWithoutScopeButton] = LOC ("inputs.help.reloadWithoutScope");
        helpText[&updateButton]             = LOC ("inputs.help.updateSnapshot");
        helpText[&editScopeButton]          = LOC ("inputs.help.editScope");
        helpText[&deleteButton]             = LOC ("inputs.help.deleteSnapshot");

        for (auto& entry : helpText)
            entry.first->addMouseListener (this, true);

        session.addListener (this);
        snapshotSessionChanged();
    }

    ~SnapshotRow() override
    {
        session.removeListener (this);
    }

    void setStatusBar (StatusBar* bar) { statusBar = bar; }

    /** The host's spacing between controls, set before setBounds. */
    void setSpacing (int pixels) { spacing = juce::jmax (0, pixels); resized(); }

    void resized() override
    {
        auto row = getLocalBounds();

        // 7 buttons + 1.5x selector = 8.5 units, sized over 8 as the Inputs tab
        // always did, with Delete taking whatever is left.
        const int buttonWidth = (row.getWidth() - spacing * 7) / 8;
        const int selectorWidth = buttonWidth * 3 / 2;

        storeButton.setBounds (row.removeFromLeft (buttonWidth));
        row.removeFromLeft (spacing);
        selector.setBounds (row.removeFromLeft (selectorWidth));
        row.removeFromLeft (spacing);
        reloadButton.setBounds (row.removeFromLeft (buttonWidth));
        row.removeFromLeft (spacing);
        reloadWithoutScopeButton.setBounds (row.removeFromLeft (buttonWidth));
        row.removeFromLeft (spacing);
        updateButton.setBounds (row.removeFromLeft (buttonWidth));
        row.removeFromLeft (spacing);
        editScopeButton.setBounds (row.removeFromLeft (buttonWidth));
        row.removeFromLeft (spacing);
        deleteButton.setBounds (row);
    }

    void mouseEnter (const juce::MouseEvent& e) override
    {
        if (statusBar == nullptr)
            return;

        // Walk up: a ComboBox hands us its internal child, not itself.
        for (auto* c = e.eventComponent; c != nullptr && c != this; c = c->getParentComponent())
        {
            const auto it = helpText.find (c);
            if (it != helpText.end())
            {
                statusBar->setHelpText (it->second);
                TTSManager::getInstance().onComponentEnter (TTSManager::extractParameterName (it->second),
                                                            TTSManager::getComponentValue (c), it->second);
                return;
            }
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (statusBar != nullptr)
            statusBar->clearText();
        TTSManager::getInstance().onComponentExit();
    }

private:
    SnapshotSession& session;
    const Family scopeFamily;
    StatusBar* statusBar = nullptr;
    int spacing = 5;
    bool rebuilding = false;

    LongPressButton storeButton;
    RefreshableComboBox selector;
    LongPressButton reloadButton;
    LongPressButton reloadWithoutScopeButton;
    LongPressButton updateButton;
    LongPressButton editScopeButton { 1 };     // 1 ms = effectively a click
    LongPressButton deleteButton;

    std::map<juce::Component*, juce::String> helpText;

    void snapshotSessionChanged() override
    {
        const juce::ScopedValueSetter<bool> guard (rebuilding, true);

        const auto names = session.getSnapshotNames();

        selector.clear (juce::dontSendNotification);
        selector.addItem (LOC ("inputs.snapshots.selectSnapshot"), 1);

        int itemId = 2;
        for (const auto& n : names)
            selector.addItem (n, itemId++);

        // The session's selection, or the placeholder so the box is never blank.
        // ComboBox::clear leaves the selection at -1, which is what used to blank
        // the selector (and disable every snapshot button) after a recall.
        if (session.hasSelection() && names.contains (session.getSelected()))
            selector.setText (session.getSelected(), juce::dontSendNotification);
        else
            selector.setSelectedId (1, juce::dontSendNotification);

        const bool hasSelection = session.hasSelection();
        reloadButton.setEnabled (hasSelection);
        updateButton.setEnabled (hasSelection);
        deleteButton.setEnabled (hasSelection);
        reloadWithoutScopeButton.setEnabled (session.canReloadWithoutScope());
    }

    bool cancelHeldSnapshotPresses() override
    {
        bool cancelled = false;
        for (auto* b : { &reloadButton, &reloadWithoutScopeButton, &updateButton, &deleteButton })
            cancelled = b->cancelPress() || cancelled;
        return cancelled;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnapshotRow)
};
