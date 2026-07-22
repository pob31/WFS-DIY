#pragma once

#include <JuceHeader.h>
#include "ColorScheme.h"

/**
 * Live "name already exists" feedback for a juce::AlertWindow name prompt.
 *
 * When the typed name collides with an existing file, the text field turns red
 * (text + outline) and a red warning line appears below it. The OK button still
 * proceeds, so the user can knowingly overwrite — this only warns, it never
 * blocks. Duplicate detection is case-insensitive so it matches the overwrite
 * behaviour of a case-insensitive filesystem (Windows/macOS).
 *
 * Usage — call after addTextEditor()/addButton() and before enterModalState(),
 * then capture the returned shared_ptr in the modal callback so the warning
 * label outlives the dialog (AlertWindow does not own custom components):
 *
 *     auto* dialog = new juce::AlertWindow (...);
 *     dialog->addTextEditor ("name", defaultName, LOC(...));
 *     dialog->addButton (LOC("common.ok"), 1, ...);
 *     dialog->addButton (LOC("common.cancel"), 0, ...);
 *     auto warning = DuplicateNameWarning::attach (*dialog, "name",
 *                        fileManager.getInputSnapshotNames(), LOC("...overwriteWarning"));
 *     dialog->enterModalState (true, juce::ModalCallbackFunction::create (
 *         [this, dialog, warning] (int result) { ...; delete dialog; }), true);
 */
namespace DuplicateNameWarning
{
    inline std::shared_ptr<juce::Label> attach (juce::AlertWindow& dialog,
                                                const juce::String& textEditorId,
                                                juce::StringArray existingNames,
                                                const juce::String& warningText)
    {
        auto* editor = dialog.getTextEditor (textEditorId);
        if (editor == nullptr)
            return nullptr;

        const auto warningColour     = ColorScheme::get().accentRed;
        const auto normalTextColour  = editor->findColour (juce::TextEditor::textColourId);
        const auto normalOutline     = editor->findColour (juce::TextEditor::outlineColourId);
        const auto normalOutlineFocus = editor->findColour (juce::TextEditor::focusedOutlineColourId);

        // Reserved warning line below the field. Always added (so the dialog
        // never resizes as the message toggles); the text is what appears and
        // disappears. Left unnamed so AlertWindow adds no extra caption row.
        auto warning = std::make_shared<juce::Label>();
        warning->setFont (juce::Font (juce::FontOptions (14.0f)));
        warning->setColour (juce::Label::textColourId, warningColour);
        warning->setJustificationType (juce::Justification::centredLeft);
        warning->setMinimumHorizontalScale (0.6f);   // let long translations shrink before truncating
        warning->setSize (360, 22);
        dialog.addCustomComponent (warning.get());

        auto validate = [editor, warning, existingNames, warningText,
                         warningColour, normalTextColour, normalOutline, normalOutlineFocus]()
        {
            const bool exists = existingNames.contains (editor->getText(), true);  // case-insensitive

            editor->applyColourToAllText (exists ? warningColour : normalTextColour, true);
            editor->setColour (juce::TextEditor::outlineColourId,        exists ? warningColour : normalOutline);
            editor->setColour (juce::TextEditor::focusedOutlineColourId, exists ? warningColour : normalOutlineFocus);
            editor->repaint();

            warning->setText (exists ? warningText : juce::String(), juce::dontSendNotification);
        };

        editor->onTextChange = validate;
        validate();  // the default name might already collide

        return warning;
    }
}
