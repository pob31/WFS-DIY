#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "DiagnosticLog.h"
#include "PluginLookAndFeel.h"

namespace wfs::plugin
{
    // Small 4-line read-only status view, polls the DiagnosticLog on a timer
    // and updates the displayed lines when the log's version counter changes.
    class StatusLogView  : public juce::Component,
                           private juce::Timer
    {
    public:
        explicit StatusLogView (const DiagnosticLog& l)  : log (l)
        {
            editor.setReadOnly (true);
            editor.setMultiLine (true, false);
            editor.setScrollbarsShown (false);
            editor.setCaretVisible (false);
            editor.setPopupMenuEnabled (false);
            editor.setColour (juce::TextEditor::backgroundColourId,
                              juce::Colour (DarkPalette::surfaceCard));
            editor.setColour (juce::TextEditor::textColourId,
                              juce::Colour (DarkPalette::textPrimary));
            editor.setColour (juce::TextEditor::outlineColourId,
                              juce::Colour (DarkPalette::chromeDivider));
            editor.setColour (juce::TextEditor::focusedOutlineColourId,
                              juce::Colour (DarkPalette::chromeDivider));
            editor.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
            addAndMakeVisible (editor);
            startTimerHz (10);
        }

        void resized() override
        {
            editor.setBounds (getLocalBounds());
        }

    private:
        void timerCallback() override
        {
            const int ver = log.getVersion();
            if (ver == lastVersion)
                return;
            lastVersion = ver;
            editor.setText (log.snapshot().joinIntoString ("\n"), juce::dontSendNotification);
        }

        const DiagnosticLog& log;
        juce::TextEditor     editor;
        int                  lastVersion = -1;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StatusLogView)
    };
}
