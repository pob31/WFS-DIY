#include "MasterEditor.h"
#include "BinaryData.h"

namespace wfs::plugin
{
    MasterEditor::MasterEditor (MasterProcessor& p)
        : juce::AudioProcessorEditor (&p), processor (p)
    {
        setLookAndFeel (&lookAndFeel);
        logoImage = juce::ImageCache::getFromMemory (BinaryData::WFSDIY_logo_png,
                                                     BinaryData::WFSDIY_logo_pngSize);
        setSize (460, 440);

        for (auto* label : { &hostLabel, &udpLabel, &httpLabel, &admLabel, &statusLabel, &tracksLabel })
            addAndMakeVisible (*label);

        hostEditor.setText ("127.0.0.1");
        udpEditor.setText ("8000");
        httpEditor.setText ("5005");
        admEditor.setText ("4001");
        for (auto* ed : { &hostEditor, &udpEditor, &httpEditor, &admEditor })
        {
            ed->setIndents (6, 4);
            addAndMakeVisible (*ed);
        }

        connectButton.onClick = [this] { onConnectClicked(); };
        addAndMakeVisible (connectButton);

        for (auto* lbl : { &hostLabel, &udpLabel, &httpLabel, &admLabel })
        {
            lbl->setFont (juce::FontOptions (14.0f));
            lbl->setColour (juce::Label::textColourId, juce::Colour (DarkPalette::textSecondary));
        }

        statusLabel.setFont (juce::FontOptions (14.0f));
        tracksLabel.setFont (juce::FontOptions (14.0f));
        statusLabel.setColour (juce::Label::textColourId, juce::Colour (DarkPalette::textSecondary));
        tracksLabel.setColour (juce::Label::textColourId, juce::Colour (DarkPalette::textSecondary));

        buildLabel.setText ("Build: " + MasterProcessor::getBuildStamp(), juce::dontSendNotification);
        buildLabel.setFont (juce::FontOptions (11.0f));
        buildLabel.setColour (juce::Label::textColourId, juce::Colour (DarkPalette::textSecondary));
        addAndMakeVisible (buildLabel);

        statusLog = std::make_unique<StatusLogView> (processor.getDiagnosticLog());
        addAndMakeVisible (*statusLog);

        startTimerHz (5);
    }

    MasterEditor::~MasterEditor()
    {
        stopTimer();
        setLookAndFeel (nullptr);
    }

    void MasterEditor::paint (juce::Graphics& g)
    {
        g.fillAll (juce::Colour (DarkPalette::background));

        auto bounds = getLocalBounds();
        auto titleArea = bounds.removeFromTop (40).reduced (14, 4);

        g.setColour (juce::Colour (DarkPalette::textPrimary));
        g.setFont (juce::FontOptions (18.0f).withStyle ("Bold"));
        g.drawFittedText ("WFS-DIY Master",
                          titleArea.removeFromLeft (titleArea.getWidth() - 120),
                          juce::Justification::centredLeft, 1);

        // Separator under title
        g.setColour (juce::Colour (DarkPalette::chromeDivider));
        g.drawHorizontalLine (44, 14.0f, static_cast<float> (getWidth() - 14));

        // Logo in bottom-right
        if (logoImage.isValid())
        {
            auto logoArea = getLocalBounds().removeFromBottom (56).removeFromRight (120).reduced (10, 6);
            g.setOpacity (0.85f);
            g.drawImageWithin (logoImage, logoArea.getX(), logoArea.getY(),
                               logoArea.getWidth(), logoArea.getHeight(),
                               juce::RectanglePlacement::xRight | juce::RectanglePlacement::yBottom
                                 | juce::RectanglePlacement::onlyReduceInSize);
            g.setOpacity (1.0f);
        }
    }

    void MasterEditor::resized()
    {
        auto area = getLocalBounds().reduced (14);
        area.removeFromTop (40);   // title bar
        area.removeFromBottom (56); // logo strip

        auto row = [&] (juce::Label& l, juce::TextEditor& e)
        {
            auto r = area.removeFromTop (28);
            l.setBounds (r.removeFromLeft (180));
            e.setBounds (r.reduced (2));
            area.removeFromTop (6);
        };

        row (hostLabel, hostEditor);
        row (udpLabel,  udpEditor);
        row (httpLabel, httpEditor);
        row (admLabel,  admEditor);

        connectButton.setBounds (area.removeFromTop (32).reduced (60, 2));
        area.removeFromTop (10);
        statusLabel.setBounds (area.removeFromTop (22));
        tracksLabel.setBounds (area.removeFromTop (22));
        area.removeFromTop (6);
        if (statusLog != nullptr)
            statusLog->setBounds (area.removeFromTop (72));
        area.removeFromTop (4);
        buildLabel.setBounds (area.removeFromTop (16));
    }

    void MasterEditor::timerCallback()
    {
        statusLabel.setText (processor.getConnectionStatus(), juce::dontSendNotification);
        tracksLabel.setText ("Registered Tracks: "
                              + juce::String (processor.getRegisteredTrackCount()),
                             juce::dontSendNotification);
        connectButton.setButtonText (processor.isConnected() ? "Disconnect" : "Connect");
    }

    void MasterEditor::onConnectClicked()
    {
        if (processor.isConnected())
        {
            processor.disconnectFromApp();
        }
        else
        {
            const auto host = hostEditor.getText();
            const auto udp  = udpEditor.getText().getIntValue();
            const auto http = httpEditor.getText().getIntValue();
            const auto adm  = admEditor.getText().getIntValue();
            processor.connectToApp (host, udp, http, adm);
        }
    }
}
