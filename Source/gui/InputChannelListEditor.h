#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "../Localization/LocalizationManager.h"
#include "../WFSLogger.h"

/**
 * Input channel list editor (stable-number model).
 *
 * One row per LIVE channel, in slot order (== number order): permanent
 * number, name, mono/stereo type selector, delete. "Add Mono" / "Add Stereo"
 * append after the last channel — numbers never shift, deleted numbers leave
 * permanent gaps. Shown from System Config (stopped-only, like every
 * structural edit); every action funnels through the WFSValueTreeState
 * structural ops and then the owner's reconfiguration callback.
 */
class InputChannelListEditor : public juce::Component,
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

        addMonoButton.setButtonText (LOC ("systemConfig.channelList.addMono"));
        addMonoButton.onClick = [this] { addChannel (false); };
        addAndMakeVisible (addMonoButton);

        addStereoButton.setButtonText (LOC ("systemConfig.channelList.addStereo"));
        addStereoButton.onClick = [this] { addChannel (true); };
        addAndMakeVisible (addStereoButton);

        statusLabel.setJustificationType (juce::Justification::centredLeft);
        statusLabel.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (statusLabel);

        refresh();
        setSize (460, 430);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10);
        auto bottom = area.removeFromBottom (34);
        listBox.setBounds (area.withTrimmedBottom (6));

        addMonoButton.setBounds (bottom.removeFromLeft (110));
        bottom.removeFromLeft (8);
        addStereoButton.setBounds (bottom.removeFromLeft (110));
        bottom.removeFromLeft (12);
        statusLabel.setBounds (bottom);
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
    // Row component
    //==========================================================================

    struct Row : public juce::Component
    {
        explicit Row (InputChannelListEditor& ownerIn) : owner (ownerIn)
        {
            numberLabel.setJustificationType (juce::Justification::centredRight);
            addAndMakeVisible (numberLabel);

            nameLabel.setJustificationType (juce::Justification::centredLeft);
            addAndMakeVisible (nameLabel);

            typeBox.addItem (LOC ("systemConfig.channelList.mono"),   1);
            typeBox.addItem (LOC ("systemConfig.channelList.stereo"), 2);
            typeBox.onChange = [this]
            {
                if (! updating)
                    owner.flipType (channelNumber, typeBox.getSelectedId() == 2);
            };
            addAndMakeVisible (typeBox);

            deleteButton.setButtonText ("x");
            deleteButton.onClick = [this] { owner.confirmDelete (channelNumber); };
            addAndMakeVisible (deleteButton);
        }

        void setSlot (int slot)
        {
            updating = true;
            channelNumber = owner.state.getInputChannelNumber (slot);
            numberLabel.setText (juce::String (channelNumber), juce::dontSendNotification);

            auto channelTree = owner.state.getInputState (slot)
                                   .getChildWithName (WFSParameterIDs::Channel);
            nameLabel.setText (channelTree.isValid()
                                   ? channelTree.getProperty (WFSParameterIDs::inputName).toString()
                                   : juce::String(),
                               juce::dontSendNotification);

            const bool stereo = owner.state.isInputChannelStereo (slot);
            typeBox.setSelectedId (stereo ? 2 : 1, juce::dontSendNotification);
            // A mono channel can only become stereo while the budget has room
            typeBox.setItemEnabled (2, stereo
                || owner.state.getNumStereoInputChannels() < WFSParameterDefaults::maxStereoChannels);
            deleteButton.setEnabled (owner.state.getNumInputChannels() > 1);
            updating = false;
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced (2);
            numberLabel.setBounds (area.removeFromLeft (36));
            area.removeFromLeft (6);
            deleteButton.setBounds (area.removeFromRight (26).reduced (0, 2));
            area.removeFromRight (6);
            typeBox.setBounds (area.removeFromRight (96).reduced (0, 2));
            area.removeFromRight (6);
            nameLabel.setBounds (area);
        }

        InputChannelListEditor& owner;
        int channelNumber = 0;
        bool updating = false;
        juce::Label numberLabel, nameLabel;
        juce::ComboBox typeBox;
        juce::TextButton deleteButton;
    };

    //==========================================================================
    // Actions (all funnel through the structural ops)
    //==========================================================================

    void addChannel (bool stereo)
    {
        auto result = state.addInputChannel (stereo);
        if (result.wasOk())
        {
            structureChanged();
            return;
        }

        // Number space exhausted with gaps available: offer explicit reuse of
        // the lowest retired number (snapshots/cues addressed to it will
        // affect the new channel — hence the confirmation).
        const int freeNumber = state.getLowestFreeChannelNumber();
        if (state.getNextChannelNumber() > WFSParameterDefaults::maxInputChannels
            && state.getNumInputChannels() < WFSParameterDefaults::maxInputChannels
            && freeNumber > 0)
        {
            auto options = juce::MessageBoxOptions()
                .withIconType (juce::MessageBoxIconType::QuestionIcon)
                .withTitle (LOC ("systemConfig.channelList.reuseTitle"))
                .withMessage (LocalizationManager::getInstance().get (
                    "systemConfig.channelList.reuseMessage",
                    {{ "number", juce::String (freeNumber) }}))
                .withButton (LOC ("systemConfig.channelList.reuse"))
                .withButton (LOC ("common.cancel"))
                .withAssociatedComponent (this);

            juce::AlertWindow::showAsync (options,
                [safeThis = juce::Component::SafePointer<InputChannelListEditor> (this),
                 stereo, freeNumber] (int choice)
                {
                    if (safeThis == nullptr || choice != 1)
                        return;
                    if (safeThis->state.addInputChannel (stereo, freeNumber).wasOk())
                        safeThis->structureChanged();
                });
            return;
        }

        WFSLogger::getInstance().logWarning ("Add input channel refused: " + result.getErrorMessage());
    }

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

    void flipType (int channelNumber, bool stereo)
    {
        if (state.setInputChannelType (channelNumber, stereo).wasOk())
            structureChanged();
        else
            refresh();  // restore the combo (budget refusal)
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

        addStereoButton.setEnabled (stereo < WFSParameterDefaults::maxStereoChannels
                                    && total < WFSParameterDefaults::maxInputChannels);
        addMonoButton.setEnabled (total < WFSParameterDefaults::maxInputChannels);
    }

    WFSValueTreeState& state;
    std::function<void()> onStructureChanged;

    juce::ListBox listBox;
    juce::TextButton addMonoButton, addStereoButton;
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputChannelListEditor)
};
