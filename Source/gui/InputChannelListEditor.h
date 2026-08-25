#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "../Localization/LocalizationManager.h"
#include "../WFSLogger.h"
#include "../WfsParameters.h"
#include "../AppSettings.h"
#include "ChannelIdentityGate.h"

/**
 * Input channel ORDER editor (stable-number model).
 *
 * One row per LIVE channel in display order: channel number, name, type
 * (read-only — a channel's type is fixed at creation; composition is edited
 * through the System Config mono/stereo counts), delete. DRAG a row to
 * rearrange the mono/stereo interleaving: the channel node and its patch row
 * move together. Stopped-only, like every structural edit.
 *
 * Numbers are permanent only once channelNumbersUserOwned has latched: a drag
 * then leaves every number untouched and a delete retires one as a gap, so the
 * patch, snapshots, QLab cues and DAW mappings never break. On a session still
 * fresh enough that no load, tab, patch window, snapshot or external controller
 * has observed a number, the structural ops renumber the whole list to display
 * order instead — so a single drag or delete can change the number of rows the
 * op never touched. Row::channelNumber is a cache read by the delete button and
 * the drag source; every op must therefore go through structureChanged(), which
 * refreshes it. Acting on a stale cached number deletes or moves the wrong
 * channel.
 *
 * The hint above the list SAYS which regime the session is in, because the two
 * feel like the same gesture and are not. Fresh: this order becomes the
 * numbering and the patch is re-flowed to match. In use: a drag moves the node
 * AND its patch row together, so every channel keeps its hardware inputs, and
 * snapshots, cues and remotes keep pointing at the same channel — the only
 * visible change is that the patch matrix stops reading as a diagonal. The
 * operator's own model had patching needing manual repair and snapshots
 * "making a mess" after a latched drag; neither is true, and the mess they saw
 * came from loading a file across an identity mismatch, which the identity
 * gate now catches. "From file..." pulls a saved channel list onto the session
 * without loading anything else (see ChannelIdentityGate::reconcileFromFile).
 */
class InputChannelListEditor : public juce::Component,
                               public juce::DragAndDropContainer,
                               public juce::DragAndDropTarget,
                               private juce::ListBoxModel
{
public:
    InputChannelListEditor (WfsParameters& parametersIn,
                            std::function<void()> onStructureChangedIn)
        : parameters (parametersIn),
          state (parametersIn.getValueTreeState()),
          onStructureChanged (std::move (onStructureChangedIn))
    {
        listBox.setModel (this);
        listBox.setRowHeight (30);
        addAndMakeVisible (listBox);

        // Multi-line: drawFittedText wraps to as many lines as the bounds hold.
        hintLabel.setJustificationType (juce::Justification::topLeft);
        hintLabel.setMinimumHorizontalScale (1.0f);
        hintLabel.setAlpha (0.8f);
        addAndMakeVisible (hintLabel);

        fromFileButton.setButtonText (LOC ("systemConfig.channelList.fromFile"));
        fromFileButton.setTooltip (LOC ("systemConfig.channelList.fromFileHelp"));
        fromFileButton.onClick = [this] { chooseFileToReconcile(); };
        addAndMakeVisible (fromFileButton);

        feedbackLabel.setJustificationType (juce::Justification::centredLeft);
        feedbackLabel.setInterceptsMouseClicks (false, false);
        feedbackLabel.setAlpha (0.8f);
        addAndMakeVisible (feedbackLabel);

        statusLabel.setJustificationType (juce::Justification::centredRight);
        statusLabel.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (statusLabel);

        refresh();
        setSize (500, 520);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10);
        hintLabel.setBounds (area.removeFromTop (74));
        area.removeFromTop (4);
        auto bottom = area.removeFromBottom (28);
        listBox.setBounds (area.withTrimmedBottom (6));
        fromFileButton.setBounds (bottom.removeFromLeft (110));
        bottom.removeFromLeft (8);
        statusLabel.setBounds (bottom.removeFromRight (150));
        feedbackLabel.setBounds (bottom);
    }

    //==========================================================================
    // DragAndDropTarget — rows are dropped back onto the editor
    //==========================================================================

    bool isInterestedInDragSource (const SourceDetails& details) override
    {
        return details.description.isInt();
    }

    void itemDropped (const SourceDetails& details) override
    {
        const int channelNumber = static_cast<int> (details.description);
        auto posInList = listBox.getLocalPoint (this, details.localPosition);
        int insertIndex = listBox.getInsertionIndexForPosition (posInList.x, posInList.y);
        if (insertIndex < 0)
            insertIndex = state.getNumInputChannels() - 1;

        // Insertion index is "before which row"; moving down needs -1 since
        // the dragged row leaves its old slot first.
        const int fromSlot = state.getSlotForChannelNumber (channelNumber);
        int targetSlot = insertIndex > fromSlot ? insertIndex - 1 : insertIndex;

        if (fromSlot >= 0 && state.moveInputChannel (channelNumber, targetSlot).wasOk())
            structureChanged();
    }

private:
    //==========================================================================
    // ListBoxModel
    //==========================================================================

    int getNumRows() override { return state.getNumInputChannels(); }

    void paintListBoxItem (int, juce::Graphics&, int, int, bool) override {}

    juce::Component* refreshComponentForRow (int rowNumber, bool,
                                             juce::Component* existing) override
    {
        auto* row = dynamic_cast<Row*> (existing);
        if (rowNumber >= getNumRows())
        {
            delete existing;
            return nullptr;
        }
        if (row == nullptr)
        {
            delete existing;
            row = new Row (*this);
        }
        row->setSlot (rowNumber);
        return row;
    }

    //==========================================================================
    // Row component (draggable)
    //==========================================================================

    struct Row : public juce::Component
    {
        explicit Row (InputChannelListEditor& ownerIn) : owner (ownerIn)
        {
            gripLabel.setText (juce::String::fromUTF8 ("\xe2\x89\xa1"), juce::dontSendNotification); // ≡
            gripLabel.setJustificationType (juce::Justification::centred);
            gripLabel.setInterceptsMouseClicks (false, false);
            addAndMakeVisible (gripLabel);

            numberLabel.setJustificationType (juce::Justification::centredRight);
            numberLabel.setInterceptsMouseClicks (false, false);
            addAndMakeVisible (numberLabel);

            nameLabel.setJustificationType (juce::Justification::centredLeft);
            nameLabel.setInterceptsMouseClicks (false, false);
            addAndMakeVisible (nameLabel);

            typeLabel.setJustificationType (juce::Justification::centredLeft);
            typeLabel.setInterceptsMouseClicks (false, false);
            typeLabel.setAlpha (0.8f);
            addAndMakeVisible (typeLabel);

            deleteButton.setButtonText ("x");
            deleteButton.onClick = [this] { owner.confirmDelete (channelNumber); };
            addAndMakeVisible (deleteButton);

            setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        }

        void setSlot (int slot)
        {
            channelNumber = owner.state.getInputChannelNumber (slot);
            numberLabel.setText (juce::String (channelNumber), juce::dontSendNotification);

            auto channelTree = owner.state.getInputState (slot)
                                   .getChildWithName (WFSParameterIDs::Channel);
            nameLabel.setText (channelTree.isValid()
                                   ? channelTree.getProperty (WFSParameterIDs::inputName).toString()
                                   : juce::String(),
                               juce::dontSendNotification);

            typeLabel.setText (owner.state.isInputChannelStereo (slot)
                                   ? LOC ("systemConfig.channelList.stereo")
                                   : LOC ("systemConfig.channelList.mono"),
                               juce::dontSendNotification);
            deleteButton.setEnabled (owner.state.getNumInputChannels() > 1);
        }

        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (! dragging && e.getDistanceFromDragStart() > 5)
            {
                dragging = true;
                owner.startDragging (juce::var (channelNumber), this);
            }
        }

        void mouseUp (const juce::MouseEvent&) override { dragging = false; }

        void resized() override
        {
            auto area = getLocalBounds().reduced (2);
            gripLabel.setBounds (area.removeFromLeft (20));
            numberLabel.setBounds (area.removeFromLeft (32));
            area.removeFromLeft (6);
            deleteButton.setBounds (area.removeFromRight (26).reduced (0, 2));
            area.removeFromRight (6);
            typeLabel.setBounds (area.removeFromRight (70));
            area.removeFromRight (6);
            nameLabel.setBounds (area);
        }

        InputChannelListEditor& owner;
        int channelNumber = 0;
        bool dragging = false;
        juce::Label gripLabel, numberLabel, nameLabel, typeLabel;
        juce::TextButton deleteButton;
    };

    //==========================================================================
    // Actions (all funnel through the structural ops)
    //==========================================================================

    void confirmDelete (int channelNumber)
    {
        auto options = juce::MessageBoxOptions()
            .withIconType (juce::MessageBoxIconType::WarningIcon)
            .withTitle (LOC ("systemConfig.channelList.deleteTitle"))
            .withMessage (LocalizationManager::getInstance().get (
                "systemConfig.channelList.deleteMessage",
                {{ "number", juce::String (channelNumber) }}))
            .withButton (LOC ("systemConfig.channelList.deleteConfirm"))
            .withButton (LOC ("common.cancel"))
            .withAssociatedComponent (this);

        juce::AlertWindow::showAsync (options,
            [safeThis = juce::Component::SafePointer<InputChannelListEditor> (this),
             channelNumber] (int choice)
            {
                if (safeThis == nullptr || choice != 1)
                    return;
                if (safeThis->state.removeInputChannel (channelNumber).wasOk())
                    safeThis->structureChanged();
            });
    }

    void structureChanged()
    {
        if (onStructureChanged)
            onStructureChanged();
        refresh();
    }

    /** "From file...": adopt a saved channel list (numbers, or order) onto this
        session without loading the file's parameters or patch. */
    void chooseFileToReconcile()
    {
        auto& fm = parameters.getFileManager();
        const juce::File start = fm.hasValidProjectFolder()
                                     ? fm.getSystemConfigFile().getParentDirectory()
                                     : AppSettings::getLastFolder ("lastXmlFolder");

        auto chooser = std::make_shared<juce::FileChooser> (LOC ("systemConfig.channelList.fromFileTitle"), start, "*.xml");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [safe = juce::Component::SafePointer<InputChannelListEditor> (this), chooser] (const juce::FileChooser& fc)
            {
                if (safe == nullptr) return;
                const auto file = fc.getResult();
                if (! file.existsAsFile()) return;
                ChannelIdentityGate::reconcileFromFile (safe->makeContext(), file);
            });
    }

    ChannelIdentityGate::Context makeContext()
    {
        juce::Component::SafePointer<InputChannelListEditor> safe (this);
        ChannelIdentityGate::Context ctx;
        ctx.parent     = this;
        ctx.parameters = &parameters;
        ctx.afterStructuralChange = [safe] { if (safe != nullptr) safe->structureChanged(); };
        ctx.showStatus = [safe] (const juce::String& text)
        {
            if (safe != nullptr) safe->feedbackLabel.setText (text, juce::dontSendNotification);
        };
        return ctx;
    }

    void refresh()
    {
        listBox.updateContent();
        listBox.repaint();

        // The regime, stated: reordering means two different things either
        // side of the latch, and the operator cannot tell which from the list.
        hintLabel.setText (LOC (state.areChannelNumbersUserOwned()
                                    ? "systemConfig.channelList.dragHintInUse"
                                    : "systemConfig.channelList.dragHintFresh"),
                           juce::dontSendNotification);

        const int total  = state.getNumInputChannels();
        const int stereo = state.getNumStereoInputChannels();
        statusLabel.setText (LocalizationManager::getInstance().get (
                                 "systemConfig.channelList.status",
                                 {{ "mono",   juce::String (total - stereo) },
                                  { "stereo", juce::String (stereo) }}),
                             juce::dontSendNotification);
    }

    WfsParameters& parameters;
    WFSValueTreeState& state;
    std::function<void()> onStructureChanged;

    juce::ListBox listBox;
    juce::Label hintLabel, feedbackLabel, statusLabel;
    juce::TextButton fromFileButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputChannelListEditor)
};
