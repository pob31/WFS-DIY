#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "MasterProcessor.h"
#include "../Shared/PluginLookAndFeel.h"

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

        PluginLookAndFeel lookAndFeel;
        juce::Image       logoImage;
        MasterProcessor&  processor;

        juce::Label       hostLabel        { {}, "Host:" };
        juce::TextEditor  hostEditor;
        juce::Label       udpLabel         { {}, "OSC send port (Tx):" };
        juce::TextEditor  udpEditor;
        juce::Label       httpLabel        { {}, "OSCQuery HTTP port:" };
        juce::TextEditor  httpEditor;
        juce::TextButton  connectButton    { "Connect" };
        juce::Label       statusLabel      { {}, "Disconnected" };
        juce::Label       tracksLabel      { {}, "Registered Tracks: 0" };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterEditor)
    };
}
