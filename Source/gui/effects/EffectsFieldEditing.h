#pragma once

#include <JuceHeader.h>
#include <optional>
#include "EffectsTabContext.h"
#include "../ColumnFocusTraverser.h"
#include "../../Helpers/TypedValue.h"

/**
    Typed values and Tab sections for the Effects panels: the behaviour the
    Inputs, Outputs and Reverb tabs give their fields, in one place because the
    Effects tab is split across five panels (EffectsTabContext says why).

    A VALUE LABEL BECOMES CLICK-TO-TYPE. Enter, Tab or a click anywhere else
    applies the number; Esc puts the old value back; either way the field
    closes. The text is read as the label shows it (TypedValue: units and
    prefixes ignored, comma decimals, "1.20 kHz", "1m 30s") - and the field's own apply
    function clamps it and writes it through the same path a drag takes, as
    one undo step. Text with no number in it changes nothing.

    TAB AND SHIFT+TAB STAY IN ONE SECTION. A section (a "circuit") is one
    column of fields on screen; Tab wraps inside it and never walks the whole
    panel. Hidden and disabled members are skipped. Label editors need a key
    listener for that, because a Label's own focus traverser ignores the
    panel's (InputsTab::CircuitTabHandler); a TextEditor reaches the panel's
    traverser, which the panel returns from createKeyboardFocusTraverser().

    A CLICK OUTSIDE ONLY CLOSES A FIELD IF THE CLICKED COMPONENT TAKES FOCUS.
    JUCE hands a click on a component that wants no focus to its parent, but
    stops at any parent of the field being edited, so a click on an empty
    patch of the field's own panel used to leave the field open. Every panel
    that holds fields therefore wants keyboard focus (setWantsKeyboardFocus),
    and so does anything between it and the tab.
*/
class EffectsFieldEditing : private juce::Label::Listener,
                            private juce::TextEditor::Listener,
                            private juce::KeyListener
{
public:
    using Apply  = std::function<void (float)>;
    using Parser = std::function<std::optional<float> (const juce::String&)>;

    /** `owner` is the panel that holds the fields: focus goes back to it
        when a field closes, so the keyboard shortcuts keep working. */
    EffectsFieldEditing (EffectsTabContext& context, juce::Component& owner)
        : ctx (context), ownerComponent (owner) {}

    /** Makes `label` click-to-type. `apply` gets the typed number, must clamp
        it, write it and leave the label showing the stored value. `editText`,
        when given, is what the field opens with instead of the label's text
        (the latency label shows a word for the sign, the field a signed number). */
    void makeEditable (juce::Label& label, const juce::String& undoName, Apply apply,
                       Parser parser = TypedValue::number, std::function<juce::String()> editText = {})
    {
        label.setEditable (true, false);
        label.addListener (this);
        entries[&label] = { undoName, std::move (apply), std::move (parser), std::move (editText), {} };
    }

    /** The panel's Tab sections, each in screen order. */
    void setCircuits (std::vector<std::vector<juce::Component*>> newCircuits)
    {
        circuits = std::move (newCircuits);
    }

    std::unique_ptr<juce::ComponentTraverser> createTraverser() const
    {
        return std::make_unique<ColumnCircuitTraverser> (circuits);
    }

    /** A TextEditor field is written when it closes only if the operator
        changed its text, so clicking into a field and out again cannot round
        a stored value through its display (a position shown as r / theta
        with one decimal). The panel's loads use setText (..., false), which
        does not count as a change. */
    void watch (juce::TextEditor& editor)         { editor.addListener (this); }
    bool takeEdited (juce::TextEditor& editor)    { return edited.erase (&editor) > 0; }
    void forgetEdit (juce::TextEditor& editor)    { edited.erase (&editor); }

    /** Enter or Esc in a TextEditor field: close it, handing the focus to
        the panel. Losing focus is what applies the field. */
    static void closeField (juce::TextEditor& editor)
    {
        for (auto* p = editor.getParentComponent(); p != nullptr; p = p->getParentComponent())
            if (p->getWantsKeyboardFocus())
            {
                p->grabKeyboardFocus();
                return;
            }
        editor.giveAwayKeyboardFocus();
    }

    //--------------------------------------------------------------------------
    // The self-test's way in: keys and focus cannot be injected from a test
    // shell, so it drives the label editors and the Tab handler directly.

    std::vector<juce::Label*> getLabelsForTest() const
    {
        std::vector<juce::Label*> labels;
        for (const auto& e : entries)
            labels.push_back (e.first);
        return labels;
    }

    juce::Label* findLabelForTest (const juce::String& undoName) const
    {
        for (const auto& e : entries)
            if (e.second.undoName == undoName)
                return e.first;
        return nullptr;
    }

    bool pressTabForTest (juce::Label& label, bool shift)
    {
        auto* editor = label.getCurrentTextEditor();
        return editor != nullptr
            && keyPressed (juce::KeyPress (juce::KeyPress::tabKey, shift ? juce::ModifierKeys::shiftModifier : 0, 0), editor);
    }

    void markEditedForTest (juce::TextEditor& editor) { edited.insert (&editor); }

private:
    struct Entry
    {
        juce::String undoName;
        Apply apply;
        Parser parser;
        std::function<juce::String()> editText;
        juce::String before;
    };

    void textEditorTextChanged (juce::TextEditor& editor) override { edited.insert (&editor); }

    void editorShown (juce::Label* label, juce::TextEditor& editor) override
    {
        auto it = entries.find (label);
        if (it == entries.end())
            return;

        it->second.before = label->getText();
        if (it->second.editText != nullptr)
        {
            editor.setText (it->second.editText(), false);
            editor.selectAll();
        }
        editor.addKeyListener (this);
    }

    void editorHidden (juce::Label*, juce::TextEditor&) override
    {
        // Give the panel the focus back once the field is gone, unless Tab
        // or a click has already put it somewhere
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<juce::Component> (&ownerComponent)]
        {
            if (safe != nullptr && safe->isShowing()
                && juce::Component::getCurrentlyFocusedComponent() == nullptr)
                safe->grabKeyboardFocus();
        });
    }

    void labelTextChanged (juce::Label* label) override
    {
        auto it = entries.find (label);
        if (it == entries.end() || ctx.isLoadingParameters)
            return;

        auto& e = it->second;
        const auto value = e.parser (label->getText());
        if (! value.has_value())
        {
            label->setText (e.before, juce::dontSendNotification);
            return;
        }

        ctx.beginGesture (e.undoName);
        e.apply (*value);
    }

    /** Tab / Shift+Tab inside a label's editor: the next field of its section. */
    bool keyPressed (const juce::KeyPress& key, juce::Component* origin) override
    {
        if (! key.isKeyCode (juce::KeyPress::tabKey))
            return false;

        auto* label = dynamic_cast<juce::Label*> (origin->getParentComponent());
        if (label == nullptr)
            return false;

        for (auto& col : circuits)
        {
            const auto it = std::find (col.begin(), col.end(), static_cast<juce::Component*> (label));
            if (it == col.end())
                continue;

            const int n = static_cast<int> (col.size());
            const int step = key.getModifiers().isShiftDown() ? n - 1 : 1;
            int idx = static_cast<int> (std::distance (col.begin(), it));
            juce::Component* next = label;

            for (int j = 0; j < n - 1; ++j)
            {
                idx = (idx + step) % n;
                auto* candidate = col[static_cast<size_t> (idx)];
                if (candidate->isVisible() && candidate->isEnabled())
                {
                    next = candidate;
                    break;
                }
            }

            label->hideEditor (false);

            if (auto* nextLabel = dynamic_cast<juce::Label*> (next))
                nextLabel->showEditor();
            else
                next->grabKeyboardFocus();

            return true;
        }

        return false;
    }

    EffectsTabContext& ctx;
    juce::Component& ownerComponent;
    std::map<juce::Label*, Entry> entries;
    std::vector<std::vector<juce::Component*>> circuits;
    std::set<juce::TextEditor*> edited;

    JUCE_DECLARE_NON_COPYABLE (EffectsFieldEditing)
};
