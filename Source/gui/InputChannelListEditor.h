#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "../Localization/LocalizationManager.h"
#include "../WFSLogger.h"

/**
 * Input channel ORDER editor (stable-number model).
 *
 * One row per LIVE channel in display order: permanent number, name, type
 * (read-only — a channel's type is fixed at creation; composition is edited
 * through the System Config mono/stereo counts), delete. DRAG a row to
 * rearrange the mono/stereo interleaving: the channel node and its patch row
 * move together, and the permanent number travels with the channel, so the
 * patch, snapshots, QLab cues and DAW mappings never break. Deleting retires
 * the number (permanent gap). Stopped-only, like every structural edit.
 */
class InputChannelListEditor : public juce::Component,
                               public juce::DragAndDropContainer,
                               public juce::DragAndDropTarget,
                               private juce::ListBoxModel
{
public:
    InputChannelListEditor (WFSValueTreeState& stateIn,
                            std::function<void()> onStructureChangedIn)
        : state (stateIn), onStructureChanged (std::move (onStructureChangedIn))
    {
        listBox.setModel (this);
        listBox.setRowHeight (30);
        addAndMakeVisible (listBox);

        hintLabel.setText (LOC ("systemConfig.channelList.dragHint"), juce::dontSendNotification);
        hintLabel.setJustificationType (juce::Justification::centredLeft);
        hintLabel.setAlpha (0.7f);
        addAndMakeVisible (hintLabel);

        statusLabel.setJustificationType (juce::Justification::centredRight);
        statusLabel.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (statusLabel);

        refresh();
        setSize (460, 430);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10);
        auto bottom = area.removeFromBottom (28);
        listBox.setBounds (area.withTrimmedBottom (6));
        statusLabel.setBounds (bottom.removeFromRight (170));
        hintLabel.setBounds (bottom);
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

    void refresh()
    {
        listBox.updateContent();
        listBox.repaint();

        const int total  = state.getNumInputChannels();
        const int stereo = state.getNumStereoInputChannels();
        statusLabel.setText (LocalizationManager::getInstance().get (
                                 "systemConfig.channelList.status",
                                 {{ "mono",   juce::String (total - stereo) },
                                  { "stereo", juce::String (stereo) }}),
                             juce::dontSendNotification);
    }

    WFSValueTreeState& state;
    std::function<void()> onStructureChanged;

    juce::ListBox listBox;
    juce::Label hintLabel, statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputChannelListEditor)
};
