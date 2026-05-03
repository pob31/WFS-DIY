#include "MasterEditor.h"
#include "BinaryData.h"

namespace wfs::plugin
{
    //==========================================================================
    // AddressMapEditor — inline editor for the Custom profile's templates.
    //==========================================================================
    class MasterEditor::AddressMapEditor : public juce::Component
    {
    public:
        AddressMapEditor (MasterProcessor& p) : processor (p)
        {
            // Reserved keys we always want a row for, in order.
            reservedKeys = { "positionX", "positionY", "positionZ",
                             "positionXYZ", "positionAed",
                             "attenuation", "directivity", "rotation",
                             "tilt", "HFshelf" };
            rebuildRows();
        }

        void rebuildRows()
        {
            rows.clear();

            auto& profile = processor.getProfileRegistry().get (TargetProfileRegistry::kIdCustom);

            std::vector<juce::String> seen;
            auto addRow = [&] (const juce::String& key, const juce::String& addr)
            {
                if (std::find (seen.begin(), seen.end(), key) != seen.end()) return;
                seen.push_back (key);
                auto row = std::make_unique<Row>();
                row->key = key;
                row->keyEditor.setText (key, juce::dontSendNotification);
                row->keyEditor.setReadOnly (false);
                row->valueEditor.setText (addr, juce::dontSendNotification);
                row->valueEditor.setIndents (6, 4);
                row->keyEditor.setIndents (6, 4);
                auto* rowPtr = row.get();
                row->valueEditor.onTextChange = [this, rowPtr] { commitRow (rowPtr); };
                row->keyEditor.onTextChange   = [this, rowPtr] { renameRow (rowPtr); };
                addAndMakeVisible (row->keyEditor);
                addAndMakeVisible (row->valueEditor);
                rows.push_back (std::move (row));
            };

            for (const auto& k : reservedKeys)
            {
                auto it = profile.paramAddressMap.find (k);
                addRow (k, it != profile.paramAddressMap.end() ? it->second : juce::String());
            }
            for (const auto& [k, v] : profile.paramAddressMap)
                addRow (k, v);

            resized();
        }

        int getPreferredHeight() const
        {
            return juce::jmax (60, static_cast<int> (rows.size()) * 26 + 6);
        }

        void resized() override
        {
            auto area = getLocalBounds();
            for (auto& row : rows)
            {
                auto r = area.removeFromTop (24);
                row->keyEditor.setBounds   (r.removeFromLeft (140).reduced (2));
                row->valueEditor.setBounds (r.reduced (2));
                area.removeFromTop (2);
            }
        }

    private:
        struct Row
        {
            juce::String key;
            juce::TextEditor keyEditor;
            juce::TextEditor valueEditor;
        };

        void commitRow (Row* row)
        {
            if (row == nullptr) return;
            auto& profile = processor.getProfileRegistry().get (TargetProfileRegistry::kIdCustom);
            const auto value = row->valueEditor.getText().trim();
            if (row->key.isEmpty()) return;
            if (value.isEmpty())
                profile.paramAddressMap.erase (row->key);
            else
                profile.paramAddressMap[row->key] = value;
        }

        void renameRow (Row* row)
        {
            if (row == nullptr) return;
            auto& profile = processor.getProfileRegistry().get (TargetProfileRegistry::kIdCustom);
            const auto oldKey = row->key;
            const auto newKey = row->keyEditor.getText().trim();
            if (newKey == oldKey || newKey.isEmpty()) return;
            const auto old = profile.paramAddressMap.find (oldKey);
            const juce::String moved = (old != profile.paramAddressMap.end()) ? old->second : row->valueEditor.getText().trim();
            if (old != profile.paramAddressMap.end())
                profile.paramAddressMap.erase (old);
            if (moved.isNotEmpty())
                profile.paramAddressMap[newKey] = moved;
            row->key = newKey;
        }

        MasterProcessor& processor;
        std::vector<juce::String>  reservedKeys;
        std::vector<std::unique_ptr<Row>> rows;
    };

    //==========================================================================
    // MasterEditor
    //==========================================================================
    MasterEditor::MasterEditor (MasterProcessor& p)
        : juce::AudioProcessorEditor (&p), processor (p)
    {
        setLookAndFeel (&lookAndFeel);
        logoImage = juce::ImageCache::getFromMemory (BinaryData::WFSDIY_logo_png,
                                                     BinaryData::WFSDIY_logo_pngSize);
        setSize (520, 600);

        // Profile selector
        addAndMakeVisible (profileLabel);
        profileLabel.setFont (juce::FontOptions (14.0f));
        profileLabel.setColour (juce::Label::textColourId, juce::Colour (DarkPalette::textSecondary));

        const auto ids = processor.getProfileRegistry().allIds();
        for (int i = 0; i < ids.size(); ++i)
        {
            const auto& id = ids[i];
            const auto& display = processor.getProfileRegistry().get (id).displayName;
            profileCombo.addItem (display, i + 1);
        }
        const int activeIdx = ids.indexOf (processor.getProfileRegistry().activeId);
        profileCombo.setSelectedId (juce::jmax (1, activeIdx + 1), juce::dontSendNotification);
        profileCombo.onChange = [this] { onProfileChanged(); };
        addAndMakeVisible (profileCombo);

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

        // ADM-OSC profile pane controls (created always; visibility set per profile).
        for (auto* lbl : { &admPaneTitle, &admWidthXLabel, &admWidthYLabel, &admWidthZLabel,
                           &admDistMaxLabel, &admEchoLabel, &customPaneTitle })
        {
            lbl->setFont (juce::FontOptions (14.0f));
            lbl->setColour (juce::Label::textColourId, juce::Colour (DarkPalette::textSecondary));
            addChildComponent (*lbl);
        }
        for (auto* ed : { &admWidthXEditor, &admWidthYEditor, &admWidthZEditor, &admDistMaxEditor })
        {
            ed->setIndents (6, 4);
            addChildComponent (*ed);
        }
        addChildComponent (admEchoToggle);

        auto pushAdmCfg = [this] (int axis, juce::TextEditor& ed)
        {
            ed.onTextChange = [this, axis, &ed]
            {
                auto& adm  = processor.getProfileRegistry().get (TargetProfileRegistry::kIdAdmOsc);
                auto& cust = processor.getProfileRegistry().get (TargetProfileRegistry::kIdCustom);
                const float w = juce::jmax (0.01f, ed.getText().getFloatValue());
                for (auto* p : { &adm, &cust })
                {
                    p->admCart.axes[axis].posInnerWidth = w;
                    p->admCart.axes[axis].posOuterWidth = w;
                    p->admCart.axes[axis].negInnerWidth = w;
                    p->admCart.axes[axis].negOuterWidth = w;
                }
            };
        };
        pushAdmCfg (0, admWidthXEditor);
        pushAdmCfg (1, admWidthYEditor);
        pushAdmCfg (2, admWidthZEditor);

        admDistMaxEditor.onTextChange = [this]
        {
            auto& adm  = processor.getProfileRegistry().get (TargetProfileRegistry::kIdAdmOsc);
            auto& cust = processor.getProfileRegistry().get (TargetProfileRegistry::kIdCustom);
            const float d = juce::jmax (0.01f, admDistMaxEditor.getText().getFloatValue());
            for (auto* p : { &adm, &cust })
            {
                p->admPolar.distInner = d * 0.5f;
                p->admPolar.distOuter = d * 0.5f;
            }
        };

        admEchoToggle.onClick = [this]
        {
            auto& profile = processor.getProfileRegistry().active();
            profile.admEchoEnabled = admEchoToggle.getToggleState();
        };

        addressMapEditor = std::make_unique<AddressMapEditor> (processor);
        addChildComponent (*addressMapEditor);

        buildLabel.setText ("Build: " + MasterProcessor::getBuildStamp(), juce::dontSendNotification);
        buildLabel.setFont (juce::FontOptions (11.0f));
        buildLabel.setColour (juce::Label::textColourId, juce::Colour (DarkPalette::textSecondary));
        addAndMakeVisible (buildLabel);

        statusLog = std::make_unique<StatusLogView> (processor.getDiagnosticLog());
        addAndMakeVisible (*statusLog);

        rebuildProfilePane();

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
        area.removeFromTop (40);    // title bar
        area.removeFromBottom (56); // logo strip

        auto row = [&] (juce::Label& l, juce::Component& e, int labelWidth = 180)
        {
            auto r = area.removeFromTop (28);
            l.setBounds (r.removeFromLeft (labelWidth));
            e.setBounds (r.reduced (2));
            area.removeFromTop (6);
        };

        // Profile selector row
        {
            auto r = area.removeFromTop (28);
            profileLabel.setBounds (r.removeFromLeft (180));
            profileCombo.setBounds (r.reduced (2));
            area.removeFromTop (6);
        }

        row (hostLabel, hostEditor);
        row (udpLabel,  udpEditor);
        if (httpLabel.isVisible()) row (httpLabel, httpEditor);
        if (admLabel.isVisible())  row (admLabel,  admEditor);

        connectButton.setBounds (area.removeFromTop (32).reduced (60, 2));
        area.removeFromTop (8);

        statusLabel.setBounds (area.removeFromTop (22));
        tracksLabel.setBounds (area.removeFromTop (22));
        area.removeFromTop (6);

        layoutProfilePane (area);

        if (statusLog != nullptr)
            statusLog->setBounds (area.removeFromTop (juce::jmax (60, area.getHeight() - 22)));
        area.removeFromTop (4);
        buildLabel.setBounds (area.removeFromTop (16));
    }

    void MasterEditor::layoutProfilePane (juce::Rectangle<int>& area)
    {
        const auto activeId = processor.getProfileRegistry().activeId;

        if (activeId == TargetProfileRegistry::kIdAdmOsc)
        {
            admPaneTitle.setBounds (area.removeFromTop (22));
            area.removeFromTop (4);
            auto rowFor = [&] (juce::Label& l, juce::TextEditor& e)
            {
                auto r = area.removeFromTop (24);
                l.setBounds (r.removeFromLeft (180));
                e.setBounds (r.reduced (2));
                area.removeFromTop (4);
            };
            rowFor (admWidthXLabel, admWidthXEditor);
            rowFor (admWidthYLabel, admWidthYEditor);
            rowFor (admWidthZLabel, admWidthZEditor);
            rowFor (admDistMaxLabel, admDistMaxEditor);
            {
                auto r = area.removeFromTop (24);
                admEchoLabel.setBounds (r.removeFromLeft (180));
                admEchoToggle.setBounds (r.reduced (2).withWidth (40));
                area.removeFromTop (8);
            }
        }
        else if (activeId == TargetProfileRegistry::kIdCustom)
        {
            customPaneTitle.setBounds (area.removeFromTop (22));
            area.removeFromTop (4);
            if (addressMapEditor != nullptr)
            {
                const int h = juce::jmin (area.getHeight() - 80, addressMapEditor->getPreferredHeight());
                addressMapEditor->setBounds (area.removeFromTop (juce::jmax (60, h)));
                area.removeFromTop (8);
            }
        }
    }

    void MasterEditor::rebuildProfilePane()
    {
        const auto activeId = processor.getProfileRegistry().activeId;
        const auto& profile = processor.getProfileRegistry().active();

        // Connection-row visibility
        const bool showHttp   = profile.showHttpField;
        const bool showAdmRx  = profile.showAdmRxField;
        httpLabel.setVisible (showHttp);
        httpEditor.setVisible (showHttp);
        admLabel.setVisible (showAdmRx);
        admEditor.setVisible (showAdmRx);

        // Profile pane visibility
        const bool isAdm = (activeId == TargetProfileRegistry::kIdAdmOsc);
        admPaneTitle.setVisible (isAdm);
        for (auto* lbl : { &admWidthXLabel, &admWidthYLabel, &admWidthZLabel, &admDistMaxLabel, &admEchoLabel })
            lbl->setVisible (isAdm);
        for (auto* ed : { &admWidthXEditor, &admWidthYEditor, &admWidthZEditor, &admDistMaxEditor })
            ed->setVisible (isAdm);
        admEchoToggle.setVisible (isAdm);

        if (isAdm)
        {
            admWidthXEditor.setText (juce::String (profile.admCart.axes[0].posInnerWidth, 1), juce::dontSendNotification);
            admWidthYEditor.setText (juce::String (profile.admCart.axes[1].posInnerWidth, 1), juce::dontSendNotification);
            admWidthZEditor.setText (juce::String (profile.admCart.axes[2].posInnerWidth, 1), juce::dontSendNotification);
            const float distMax = profile.admPolar.distInner + profile.admPolar.distOuter;
            admDistMaxEditor.setText (juce::String (juce::jmax (0.1f, distMax), 1), juce::dontSendNotification);
            admEchoToggle.setToggleState (profile.admEchoEnabled, juce::dontSendNotification);
        }

        const bool isCustom = (activeId == TargetProfileRegistry::kIdCustom);
        customPaneTitle.setVisible (isCustom);
        if (addressMapEditor != nullptr)
        {
            addressMapEditor->setVisible (isCustom);
            if (isCustom) addressMapEditor->rebuildRows();
        }

        resized();
    }

    void MasterEditor::onProfileChanged()
    {
        const int idx = profileCombo.getSelectedId() - 1;
        const auto ids = processor.getProfileRegistry().allIds();
        if (idx < 0 || idx >= ids.size()) return;
        processor.getProfileRegistry().activeId = ids[idx];
        rebuildProfilePane();
    }

    void MasterEditor::timerCallback()
    {
        statusLabel.setText (processor.getConnectionStatus(), juce::dontSendNotification);
        tracksLabel.setText ("Registered Tracks: "
                              + juce::String (processor.getRegisteredTrackCount()),
                             juce::dontSendNotification);
        connectButton.setButtonText (processor.isConnected() ? "Disconnect" : "Connect");

        // If state was loaded after the editor was constructed (e.g. DAW recall),
        // sync the combobox to reflect the restored active profile.
        const auto& reg = processor.getProfileRegistry();
        const int wantedIdx = reg.allIds().indexOf (reg.activeId);
        if (wantedIdx >= 0 && profileCombo.getSelectedId() != wantedIdx + 1)
        {
            profileCombo.setSelectedId (wantedIdx + 1, juce::dontSendNotification);
            rebuildProfilePane();
        }
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
