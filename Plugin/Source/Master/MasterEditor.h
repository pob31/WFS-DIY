#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "MasterProcessor.h"
#include "../Shared/PluginLookAndFeel.h"
#include "../Shared/StatusLogView.h"

namespace wfs::plugin
{
    class MasterEditor  : public juce::AudioProcessorEditor,
                          private juce::Timer
    {
    public:
        explicit MasterEditor (MasterProcessor&);
        ~MasterEditor() override;

        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        void timerCallback() override;
        void onConnectClicked();
        void onProfileChanged();
        void rebuildProfilePane();
        void layoutProfilePane (juce::Rectangle<int>& area);

        // Custom profile address-map editor: simple table-like list of rows.
        class AddressMapEditor;

        PluginLookAndFeel lookAndFeel;
        juce::Image       logoImage;
        MasterProcessor&  processor;

        juce::Label       profileLabel     { {}, "Target profile:" };
        juce::ComboBox    profileCombo;

        juce::Label       hostLabel        { {}, "Host:" };
        juce::TextEditor  hostEditor;
        juce::Label       udpLabel         { {}, "OSC send port (Tx):" };
        juce::TextEditor  udpEditor;
        juce::Label       httpLabel        { {}, "OSCQuery HTTP port:" };
        juce::TextEditor  httpEditor;
        juce::Label       admLabel         { {}, "ADM-OSC Rx port:" };
        juce::TextEditor  admEditor;
        juce::TextButton  connectButton    { "Connect" };
        juce::Label       statusLabel      { {}, "Disconnected" };
        juce::Label       tracksLabel      { {}, "Registered Tracks: 0" };
        juce::Label       buildLabel;

        // ADM-OSC profile inline pane: four symmetric width inputs.
        juce::Label       admPaneTitle    { {}, "ADM-OSC field size (symmetric, metres):" };
        juce::Label       admWidthXLabel  { {}, "X half-width:" };
        juce::TextEditor  admWidthXEditor;
        juce::Label       admWidthYLabel  { {}, "Y half-width:" };
        juce::TextEditor  admWidthYEditor;
        juce::Label       admWidthZLabel  { {}, "Z half-width:" };
        juce::TextEditor  admWidthZEditor;
        juce::Label       admDistMaxLabel { {}, "Polar dist max:" };
        juce::TextEditor  admDistMaxEditor;
        juce::Label       admEchoLabel    { {}, "Listen for ADM echoes:" };
        juce::ToggleButton admEchoToggle  { "" };

        // Custom profile inline pane: address-template editor.
        std::unique_ptr<AddressMapEditor> addressMapEditor;
        juce::Label       customPaneTitle  { {}, "Custom address templates ({id} = channel):" };

        std::unique_ptr<StatusLogView> statusLog;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterEditor)
    };
}
