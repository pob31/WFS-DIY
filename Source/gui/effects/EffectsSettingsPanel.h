#pragma once

#include <JuceHeader.h>
#include "EffectsTabContext.h"
#include "../ColorScheme.h"
#include "../buttons/LongPressButton.h"
#include "../sliders/WfsStandardSlider.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../../Parameters/WFSParameterDefaults.h"
#include "../../Localization/LocalizationManager.h"
#if WFS_GPU_NATIVE
 #include "../../../spatcore/gpu/GpuDeviceManager.h"
#endif

/**
    The Settings sub-tab: the nine effectsGlobal* settings and the Re-layout
    button.

    THESE ARE CONFIG, NOT CHANNEL PARAMETERS. They live on
    <Config><EffectsGlobal>, are written through setConfigParam (the main undo
    manager, no link funnel - there is no channel to propagate from) and,
    apart from the loop-guard switch, are read by the engine in prepare: they
    apply at the next Processing start, and the panel says so.

    THE LINK-GROUP NAMES are the one setting the rest of the tab reads: the
    Channel Parameters combo, the Chain badge and the sends matrix all name
    groups from effectsGlobalLinkNames, so the tab refreshes them when the
    list changes here.

    RE-LAYOUT is the app placing the returns, not the operator: it re-runs
    the ring placement over every channel and clears the effects-only
    ownership latch, as the CSV's header row specifies.
*/
class EffectsSettingsPanel : public juce::Component,
                             private juce::TextEditor::Listener
{
public:
    static constexpr int numGroups = WFSParameterDefaults::effectLinkGroupMax;

    explicit EffectsSettingsPanel (EffectsTabContext& context) : ctx (context)
    {
        // ---- Link groups --------------------------------------------------
        addTitle (linkTitle, "effects.settings.linkGroups");

        for (int g = 0; g < numGroups; ++g)
        {
            addLabel (groupLabels[g], LOC ("effects.settings.group").replace ("{n}", juce::String (g + 1)));
            auto& ed = groupEditors[g];
            ed.setMultiLine (false);
            ed.setInputRestrictions (32, juce::String());
            ed.addListener (this);
            addAndMakeVisible (ed);
            ctx.helpTextMap[&ed] = LOC ("effects.help.settingsLinkNames");
        }

        addLabel (linkModeLabel, LOC ("effects.settings.defaultLinkMode"));
        addAndMakeVisible (linkModeCombo);
        linkModeCombo.addItem (LOC ("effects.link.modeOff"), 1);
        linkModeCombo.addItem (LOC ("effects.link.modeAbsolute"), 2);
        linkModeCombo.addItem (LOC ("effects.link.modeRelative"), 3);
        linkModeCombo.onChange = [this]
        {
            if (ctx.isLoadingParameters) return;
            writeConfig ("effectsGlobalLinkMode", linkModeCombo.getSelectedId() - 1, "Effects Default Link Mode");
        };
        ctx.helpTextMap[&linkModeCombo] = LOC ("effects.help.settingsLinkMode");

        addAndMakeVisible (relayoutButton);
        relayoutButton.setButtonText (LOC ("effects.settings.relayout"));
        relayoutButton.setBaseColour (juce::Colour (0xFF3A6EA5));
        relayoutButton.onLongPress = [this]
        {
            auto& vts = ctx.parameters.getValueTreeState();
            vts.redistributeAllEffectPositions();
            // Back to app-owned positions: a channel-count change re-lays the ring again
            vts.getEffectsState().setProperty (WFSParameterIDs::effectPositionsUserOwned, 0, nullptr);
            ctx.showStatusMessage (LOC ("effects.messages.relayoutDone"));
        };
        ctx.helpTextMap[&relayoutButton] = LOC ("effects.help.settingsRelayout");

        // ---- Engine -------------------------------------------------------
        addTitle (engineTitle, "effects.settings.engine");

        addLabel (fxFeedLabel, LOC ("effects.settings.fxFeed"));
        addAndMakeVisible (fxFeedButton);
        fxFeedButton.onClick = [this]
        {
            const bool geometric = readInt ("effectsGlobalFxFeedGeometric", WFSParameterDefaults::effectsGlobalFxFeedGeometricDefault) != 0;
            writeConfig ("effectsGlobalFxFeedGeometric", geometric ? 0 : 1, "Effects Feed Geometry");
            loadParameters();
        };
        ctx.helpTextMap[&fxFeedButton] = LOC ("effects.help.settingsFxFeed");

        addLabel (workerThreadsLabel, LOC ("effects.settings.workerThreads"));
        setupNumberBox (workerThreadsEditor, true);
        ctx.helpTextMap[&workerThreadsEditor] = LOC ("effects.help.settingsWorkerThreads");

        addLabel (cushionLabel, LOC ("effects.settings.returnCushion"));
        addAndMakeVisible (cushionCombo);
        cushionCombo.addItem (LOC ("effects.settings.cushion.auto"), 1);
        cushionCombo.addItem (LOC ("effects.settings.cushion.n1"), 2);
        cushionCombo.addItem (LOC ("effects.settings.cushion.n2"), 3);
        cushionCombo.addItem (LOC ("effects.settings.cushion.n3"), 4);
        cushionCombo.onChange = [this]
        {
            if (ctx.isLoadingParameters) return;
            writeConfig ("effectsGlobalReturnCushion", cushionCombo.getSelectedId() - 1, "Effects Return Cushion");
        };
        ctx.helpTextMap[&cushionCombo] = LOC ("effects.help.settingsReturnCushion");

        addLabel (loopGuardLabel, LOC ("effects.settings.loopGuard"));
        addAndMakeVisible (loopGuardButton);
        loopGuardButton.onClick = [this]
        {
            const bool on = readInt ("effectsGlobalLoopGuard", WFSParameterDefaults::effectsGlobalLoopGuardDefault) != 0;
            writeConfig ("effectsGlobalLoopGuard", on ? 0 : 1, "Effects Loop Guard");
            loadParameters();
        };
        ctx.helpTextMap[&loopGuardButton] = LOC ("effects.help.settingsLoopGuard");

        addLabel (ceilingLabel, LOC ("effects.settings.loopGuardCeiling"));
        ceilingSlider.setTrackColours (juce::Colour (0xFF1E1E1E), juce::Colour (0xFFFF5722));
        ceilingSlider.onGestureStart = [this] { ctx.beginGesture ("Effects Loop Guard Ceiling"); };
        ceilingSlider.onValueChanged = [this] (float v)
        {
            namespace D = WFSParameterDefaults;
            const float dB = D::effectsGlobalLoopGuardCeilingMin + (D::effectsGlobalLoopGuardCeilingMax - D::effectsGlobalLoopGuardCeilingMin) * v;
            ceilingValue.setText (juce::String (dB, 1) + " dBFS", juce::dontSendNotification);
            if (! ctx.isLoadingParameters)
                ctx.parameters.setConfigParam ("effectsGlobalLoopGuardCeiling", dB);
        };
        addAndMakeVisible (ceilingSlider);
        addLabel (ceilingValue, "6.0 dBFS");
        ceilingValue.setJustificationType (juce::Justification::centredRight);
        ctx.helpTextMap[&ceilingSlider] = LOC ("effects.help.settingsLoopGuardCeiling");

        addLabel (maxDelayLabel, LOC ("effects.settings.maxDelay"));
        setupNumberBox (maxDelayEditor, false);
        ctx.helpTextMap[&maxDelayEditor] = LOC ("effects.help.settingsMaxDelay");
        addLabel (maxDelayUnit, LOC ("units.seconds"));

        addLabel (deviceLabel, LOC ("effects.settings.feedDevice"));
        addAndMakeVisible (deviceCombo);
        buildDeviceList();
        for (int i = 0; i < static_cast<int> (devices.size()); ++i)
            deviceCombo.addItem (devices[static_cast<size_t> (i)].second, i + 1);
        deviceCombo.onChange = [this]
        {
            if (ctx.isLoadingParameters) return;
            const int id = deviceCombo.getSelectedId();
            const auto devId = id >= 1 && id <= static_cast<int> (devices.size())
                                 ? devices[static_cast<size_t> (id - 1)].first : juce::String ("cpu");
            writeConfig ("effectsGlobalFeedGpuDevice", devId, "Effects Feed Device");
        };
        ctx.helpTextMap[&deviceCombo] = LOC ("effects.help.settingsFeedDevice");

        addAndMakeVisible (noteLabel);
        noteLabel.setText (LOC ("effects.settings.note"), juce::dontSendNotification);
        noteLabel.setColour (juce::Label::textColourId, ColorScheme::get().textDisabled);
        noteLabel.setJustificationType (juce::Justification::topLeft);
    }

    void loadParameters()
    {
        namespace D = WFSParameterDefaults;
        const juce::ScopedValueSetter<bool> loadingScope (ctx.isLoadingParameters, true);

        juce::StringArray names;
        names.addTokens (ctx.parameters.getConfigParam ("effectsGlobalLinkNames").toString(), ",", "");
        for (int g = 0; g < numGroups; ++g)
            groupEditors[g].setText (names[g].trim(), false);

        linkModeCombo.setSelectedId (juce::jlimit (0, 2, readInt ("effectsGlobalLinkMode", D::effectsGlobalLinkModeDefault)) + 1,
                                     juce::dontSendNotification);

        const bool geometric = readInt ("effectsGlobalFxFeedGeometric", D::effectsGlobalFxFeedGeometricDefault) != 0;
        fxFeedButton.setButtonText (LOC (geometric ? "effects.settings.fxFeedGeometric" : "effects.settings.fxFeedMatrix"));
        fxFeedButton.setColour (juce::TextButton::buttonColourId,
                                geometric ? juce::Colour (0xFF26A69A) : ColorScheme::get().buttonNormal);

        workerThreadsEditor.setText (juce::String (readInt ("effectsGlobalWorkerThreads", D::effectsGlobalWorkerThreadsDefault)), false);

        cushionCombo.setSelectedId (juce::jlimit (0, 3, readInt ("effectsGlobalReturnCushion", D::effectsGlobalReturnCushionDefault)) + 1,
                                    juce::dontSendNotification);

        const bool guard = readInt ("effectsGlobalLoopGuard", D::effectsGlobalLoopGuardDefault) != 0;
        loopGuardButton.setButtonText (LOC (guard ? "effects.settings.loopGuardOn" : "effects.settings.loopGuardOff"));
        loopGuardButton.setColour (juce::TextButton::buttonColourId,
                                   guard ? juce::Colour (0xFF26A69A) : ColorScheme::get().buttonNormal);

        const auto cv = ctx.parameters.getConfigParam ("effectsGlobalLoopGuardCeiling");
        const float ceiling = juce::jlimit (D::effectsGlobalLoopGuardCeilingMin, D::effectsGlobalLoopGuardCeilingMax,
                                            cv.isVoid() ? D::effectsGlobalLoopGuardCeilingDefault : static_cast<float> (static_cast<double> (cv)));
        ceilingSlider.setValue ((ceiling - D::effectsGlobalLoopGuardCeilingMin)
                                / (D::effectsGlobalLoopGuardCeilingMax - D::effectsGlobalLoopGuardCeilingMin));
        ceilingValue.setText (juce::String (ceiling, 1) + " dBFS", juce::dontSendNotification);

        maxDelayEditor.setText (juce::String (readInt ("effectsGlobalMaxDelaySeconds", D::effectsGlobalMaxDelaySecondsDefault)), false);

        const auto devId = ctx.parameters.getConfigParam ("effectsGlobalFeedGpuDevice").toString();
        int comboId = 1;
        for (int i = 0; i < static_cast<int> (devices.size()); ++i)
            if (devices[static_cast<size_t> (i)].first == devId) { comboId = i + 1; break; }
        deviceCombo.setSelectedId (comboId, juce::dontSendNotification);
    }

    void resized() override
    {
        layoutScale = static_cast<float> (getHeight()) / 780.0f;
        auto area = getLocalBounds().reduced (scaled (10), scaled (10));
        const int rowH = scaled (30);
        const int sliderH = scaled (35);
        const int spacing = scaled (8);
        const int labelW = scaled (190);
        const int valueW = scaled (90);
        const int titleH = scaled (25);
        const int controlW = scaled (220);

        auto col1 = area.removeFromLeft (area.getWidth() / 2).reduced (scaled (5), 0);
        auto col2 = area.reduced (scaled (5), 0);

        // ---- Link groups (left) ----
        linkTitle.setBounds (col1.removeFromTop (titleH));
        col1.removeFromTop (spacing);
        for (int g = 0; g < numGroups; ++g)
        {
            auto row = col1.removeFromTop (rowH);
            groupLabels[g].setBounds (row.removeFromLeft (scaled (90)));
            groupEditors[g].setBounds (row.removeFromLeft (controlW).reduced (0, scaled (3)));
            col1.removeFromTop (scaled (4));
        }
        col1.removeFromTop (spacing);
        {
            auto row = col1.removeFromTop (rowH);
            linkModeLabel.setBounds (row.removeFromLeft (labelW));
            linkModeCombo.setBounds (row.removeFromLeft (controlW));
        }
        col1.removeFromTop (spacing * 3);
        relayoutButton.setBounds (col1.removeFromTop (rowH).removeFromLeft (labelW + controlW));

        // ---- Engine (right) ----
        engineTitle.setBounds (col2.removeFromTop (titleH));
        col2.removeFromTop (spacing);

        auto row = col2.removeFromTop (rowH);
        fxFeedLabel.setBounds (row.removeFromLeft (labelW));
        fxFeedButton.setBounds (row.removeFromLeft (controlW));
        col2.removeFromTop (spacing);

        row = col2.removeFromTop (rowH);
        workerThreadsLabel.setBounds (row.removeFromLeft (labelW));
        workerThreadsEditor.setBounds (row.removeFromLeft (scaled (80)).reduced (0, scaled (3)));
        col2.removeFromTop (spacing);

        row = col2.removeFromTop (rowH);
        cushionLabel.setBounds (row.removeFromLeft (labelW));
        cushionCombo.setBounds (row.removeFromLeft (controlW));
        col2.removeFromTop (spacing);

        row = col2.removeFromTop (rowH);
        loopGuardLabel.setBounds (row.removeFromLeft (labelW));
        loopGuardButton.setBounds (row.removeFromLeft (controlW));
        col2.removeFromTop (spacing);

        row = col2.removeFromTop (rowH);
        ceilingLabel.setBounds (row.removeFromLeft (labelW));
        ceilingValue.setBounds (row.removeFromRight (valueW));
        col2.removeFromTop (scaled (3));
        ceilingSlider.setBounds (col2.removeFromTop (sliderH));
        col2.removeFromTop (spacing);

        row = col2.removeFromTop (rowH);
        maxDelayLabel.setBounds (row.removeFromLeft (labelW));
        maxDelayEditor.setBounds (row.removeFromLeft (scaled (80)).reduced (0, scaled (3)));
        row.removeFromLeft (scaled (6));
        maxDelayUnit.setBounds (row.removeFromLeft (scaled (40)));
        col2.removeFromTop (spacing);

        row = col2.removeFromTop (rowH);
        deviceLabel.setBounds (row.removeFromLeft (labelW));
        deviceCombo.setBounds (row.removeFromLeft (controlW));
        col2.removeFromTop (spacing * 2);

        noteLabel.setBounds (col2.removeFromTop (scaled (44)));
    }

private:
    void buildDeviceList()
    {
        devices.clear();
        devices.push_back ({ "cpu", "CPU" });
#if WFS_GPU_NATIVE
        for (const auto& d : GpuDeviceManager::instance().devices())
            if (! d.isCpu())
                devices.push_back ({ juce::String (d.id), juce::String (d.name) });
#endif
    }

    int readInt (const char* name, int fallback) const
    {
        const auto v = ctx.parameters.getConfigParam (name);
        return v.isVoid() ? fallback : static_cast<int> (v);
    }

    void writeConfig (const char* name, const juce::var& value, const juce::String& gestureName)
    {
        ctx.beginGesture (gestureName);
        ctx.parameters.setConfigParam (name, value);
    }

    void setupNumberBox (juce::TextEditor& ed, bool allowNegative)
    {
        ed.setMultiLine (false);
        ed.setInputRestrictions (4, allowNegative ? "-0123456789" : "0123456789");
        ed.setJustification (juce::Justification::centredRight);
        ed.setSelectAllWhenFocused (true);
        ed.addListener (this);
        addAndMakeVisible (ed);
    }

    void textEditorReturnKeyPressed (juce::TextEditor& editor) override { commit (editor); }
    void textEditorFocusLost (juce::TextEditor& editor) override        { commit (editor); }

    void commit (juce::TextEditor& editor)
    {
        if (ctx.isLoadingParameters)
            return;
        namespace D = WFSParameterDefaults;

        for (int g = 0; g < numGroups; ++g)
            if (&editor == &groupEditors[g])
            {
                // One comma-separated list; a blank name falls back to "Group n"
                juce::StringArray names;
                for (int i = 0; i < numGroups; ++i)
                {
                    auto name = groupEditors[i].getText().trim().replaceCharacter (',', ' ');
                    names.add (name.isNotEmpty() ? name : LOC ("effects.link.group") + " " + juce::String (i + 1));
                }
                writeConfig ("effectsGlobalLinkNames", names.joinIntoString (","), "Effects Link Group Names");
                return;
            }

        if (&editor == &workerThreadsEditor)
        {
            const int v = juce::jlimit (D::effectsGlobalWorkerThreadsMin, D::effectsGlobalWorkerThreadsMax, editor.getText().getIntValue());
            editor.setText (juce::String (v), false);
            writeConfig ("effectsGlobalWorkerThreads", v, "Effects Worker Threads");
        }
        else if (&editor == &maxDelayEditor)
        {
            const int v = juce::jlimit (D::effectsGlobalMaxDelaySecondsMin, D::effectsGlobalMaxDelaySecondsMax, editor.getText().getIntValue());
            editor.setText (juce::String (v), false);
            writeConfig ("effectsGlobalMaxDelaySeconds", v, "Effects Maximum Delay");
        }
    }

    void addTitle (juce::Label& label, const char* key)
    {
        label.setText (LOC (key), juce::dontSendNotification);
        label.setFont (juce::FontOptions (16.0f).withStyle ("Bold"));
        addAndMakeVisible (label);
    }

    void addLabel (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (label);
    }

    int scaled (int ref) const
    {
        return juce::jmax (static_cast<int> (ref * 0.65f), static_cast<int> (ref * layoutScale));
    }

    EffectsTabContext& ctx;
    float layoutScale = 1.0f;
    std::vector<std::pair<juce::String, juce::String>> devices;   // id, name

    juce::Label linkTitle, engineTitle, noteLabel;
    juce::Label groupLabels[numGroups];
    juce::TextEditor groupEditors[numGroups];
    juce::Label linkModeLabel;
    juce::ComboBox linkModeCombo;
    LongPressButton relayoutButton { 800 };

    juce::Label fxFeedLabel, workerThreadsLabel, cushionLabel, loopGuardLabel, ceilingLabel, ceilingValue;
    juce::Label maxDelayLabel, maxDelayUnit, deviceLabel;
    juce::TextButton fxFeedButton, loopGuardButton;
    juce::TextEditor workerThreadsEditor, maxDelayEditor;
    juce::ComboBox cushionCombo, deviceCombo;
    WfsStandardSlider ceilingSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectsSettingsPanel)
};
