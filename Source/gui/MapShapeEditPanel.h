#pragma once

#include <JuceHeader.h>
#include <map>
#include "MapShapesEditor.h"
#include "InputColourPicker.h"
#include "ColorScheme.h"
#include "WfsLookAndFeel.h"
#include "StatusBar.h"
#include "../Localization/LocalizationManager.h"

/**
 * The draw-tools panel the Map shows in shape-edit mode: tools, outline
 * style (colour, width, dashes), the label of the selected shape, and
 * copy / paste / delete. Every control drives the MapShapesEditor; the panel
 * re-reads it through refresh() whenever the selection or clipboard changes.
 *
 * None of the buttons take keyboard focus, so Delete, arrows and the editing
 * shortcuts keep reaching the Map. The label field is the one exception.
 */
class MapShapeEditPanel : public juce::Component,
                          private MapDrawing::ShapeClipboard::Listener
{
public:
    explicit MapShapeEditPanel (MapShapesEditor& ed) : editor (ed)
    {
        using Tool = MapShapesEditor::Tool;
        const std::pair<juce::TextButton*, std::pair<Tool, const char*>> tools[] = {
            { &selectBtn,    { Tool::Select,     "map.shapes.tools.select" } },
            { &pointsBtn,    { Tool::EditPoints, "map.shapes.tools.editPoints" } },
            { &lineBtn,      { Tool::Line,       "map.shapes.tools.line" } },
            { &polygonBtn,   { Tool::Polygon,    "map.shapes.tools.polygon" } },
            { &rectBtn,      { Tool::Rectangle,  "map.shapes.tools.rectangle" } },
            { &ellipseBtn,   { Tool::Ellipse,    "map.shapes.tools.ellipse" } },
            { &freehandBtn,  { Tool::Freehand,   "map.shapes.tools.freehand" } },
        };
        for (auto& [btn, info] : tools)
        {
            const auto t = info.first;
            btn->setButtonText (LOC (info.second));
            btn->setRadioGroupId (4711);
            btn->setClickingTogglesState (true);
            btn->onClick = [this, t] { editor.setTool (t); refresh(); };
            addAndMakeVisible (*btn);
        }

        for (int i = 0; i < numSwatches; ++i)
        {
            auto* sw = swatches.add (new Swatch (juce::Colour (swatchColours[i])));
            sw->onClick = [this, c = juce::Colour (swatchColours[i])] { editor.setColour (c); refresh(); };
            addAndMakeVisible (sw);
        }
        moreColoursBtn.setButtonText (juce::String::charToString (0x2026));   // …
        moreColoursBtn.onClick = [this]
        {
            juce::Component::SafePointer<MapShapeEditPanel> safe (this);
            WfsInputColourPicker::show (moreColoursBtn, editor.getDisplayedStyle().colour,
                [safe] (juce::Colour c) { if (safe != nullptr) { safe->editor.setColour (c, false); safe->refresh(); } },
                [safe] { if (safe != nullptr) { safe->editor.setColour (juce::Colour (swatchColours[1])); safe->refresh(); } },
                [safe] { if (safe != nullptr) safe->editor.beginStyleGesture(); });
        };
        addAndMakeVisible (moreColoursBtn);

        widthLabel.setText (LOC ("map.shapes.style.width"), juce::dontSendNotification);
        widthLabel.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (widthLabel);
        for (int i = 0; i < (int) std::size (widths); ++i)
            widthCombo.addItem (juce::String (widths[i], widths[i] < 1.0f || widths[i] != std::floor (widths[i]) ? 1 : 0) + " px", i + 1);
        widthCombo.onChange = [this]
        {
            if (updating) return;
            const int i = widthCombo.getSelectedId() - 1;
            if (i >= 0 && i < (int) std::size (widths))
                editor.setLineWidth (widths[i]);
        };
        addAndMakeVisible (widthCombo);

        dashedBtn.setButtonText (LOC ("map.shapes.style.dashed"));
        dashedBtn.setClickingTogglesState (true);
        dashedBtn.onClick = [this] { editor.setDashed (dashedBtn.getToggleState()); };
        addAndMakeVisible (dashedBtn);

        labelEditor.setTextToShowWhenEmpty (LOC ("map.shapes.style.labelPlaceholder"),
                                            ColorScheme::get().textDisabled);
        labelEditor.setInputRestrictions (MapDrawing::maxLabelLength);
        labelEditor.onReturnKey = [this] { commitLabel(); unfocusAllComponents(); };
        labelEditor.onEscapeKey = [this] { refresh(); unfocusAllComponents(); };
        labelEditor.onFocusLost = [this] { commitLabel(); };
        addAndMakeVisible (labelEditor);

        copyBtn.setButtonText (LOC ("map.shapes.actions.copy"));
        copyBtn.onClick = [this] { editor.copySelection(); };
        pasteBtn.onClick = [this] { editor.paste(); };
        deleteBtn.setButtonText (LOC ("map.shapes.actions.delete"));
        deleteBtn.onClick = [this] { editor.deleteSelection(); };
        for (auto* b : { &copyBtn, &pasteBtn, &deleteBtn })
            addAndMakeVisible (*b);

        for (auto* c : getChildren())
            if (c != &labelEditor)
                c->setWantsKeyboardFocus (false);

        setupHelpText();
        MapDrawing::ShapeClipboard::get().addListener (this);
        refresh();
    }

    ~MapShapeEditPanel() override
    {
        MapDrawing::ShapeClipboard::get().removeListener (this);
    }

    void setStatusBar (StatusBar* bar) { statusBar = bar; }

    /** Re-read tool, style, label and clipboard state from the editor. */
    void refresh()
    {
        const juce::ScopedValueSetter<bool> guard (updating, true);
        using Tool = MapShapesEditor::Tool;
        const auto tool = editor.getTool();
        selectBtn.setToggleState   (tool == Tool::Select,     juce::dontSendNotification);
        pointsBtn.setToggleState   (tool == Tool::EditPoints, juce::dontSendNotification);
        lineBtn.setToggleState     (tool == Tool::Line,       juce::dontSendNotification);
        polygonBtn.setToggleState  (tool == Tool::Polygon,    juce::dontSendNotification);
        rectBtn.setToggleState     (tool == Tool::Rectangle,  juce::dontSendNotification);
        ellipseBtn.setToggleState  (tool == Tool::Ellipse,    juce::dontSendNotification);
        freehandBtn.setToggleState (tool == Tool::Freehand,   juce::dontSendNotification);

        const auto style = editor.getDisplayedStyle();
        for (auto* sw : swatches)
            sw->setSelected (sw->colour.getARGB() == style.colour.getARGB());
        moreColoursBtn.setColour (juce::TextButton::buttonColourId, style.colour.withAlpha (0.8f));

        int bestId = 0;
        float bestDiff = 1.0e9f;
        for (int i = 0; i < (int) std::size (widths); ++i)
        {
            const float diff = std::abs (widths[i] - style.lineWidth);
            if (diff < bestDiff) { bestDiff = diff; bestId = i + 1; }
        }
        widthCombo.setSelectedId (bestId, juce::dontSendNotification);
        dashedBtn.setToggleState (style.dashed, juce::dontSendNotification);

        const bool hasSel = editor.hasSelection();
        if (! labelEditor.hasKeyboardFocus (true))
            labelEditor.setText (editor.getDisplayedLabel(), juce::dontSendNotification);
        labelEditor.setEnabled (hasSel);
        copyBtn.setEnabled (hasSel);
        deleteBtn.setEnabled (hasSel);

        using Kind = MapDrawing::ShapeClipboard::Kind;
        const auto kind = MapDrawing::ShapeClipboard::get().getKind();
        pasteBtn.setButtonText (kind == Kind::GradientShapes || kind == Kind::GradientLayer
                                    ? LOC ("map.shapes.actions.pasteFromGradient")
                                    : LOC ("map.shapes.actions.paste"));
        pasteBtn.setEnabled (kind != Kind::None);
        repaint();
    }

    int getIdealHeight (int width) const
    {
        juce::ignoreUnused (width);
        const int row = rowHeight(), gap = gapSize(), pad = padSize();
        // 4 tool rows, swatches, width+dashed, label, copy+paste, delete
        return pad * 2 + row * 9 + gap * 8 + gap * 2;   // two section spacers
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (ColorScheme::get().surfaceCard.withAlpha (0.94f));
        g.fillRoundedRectangle (b, 6.0f);
        g.setColour (ColorScheme::get().buttonBorder);
        g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);
    }

    void resized() override
    {
        const int row = rowHeight(), gap = gapSize(), pad = padSize();
        auto area = getLocalBounds().reduced (pad);

        auto twoUp = [&] (juce::Component& a, juce::Component* b)
        {
            auto r = area.removeFromTop (row);
            area.removeFromTop (gap);
            const int half = (r.getWidth() - gap) / 2;
            a.setBounds (r.removeFromLeft (half));
            r.removeFromLeft (gap);
            if (b != nullptr) b->setBounds (r);
        };

        twoUp (selectBtn, &pointsBtn);
        twoUp (lineBtn, &polygonBtn);
        twoUp (rectBtn, &ellipseBtn);
        twoUp (freehandBtn, nullptr);
        area.removeFromTop (gap);

        {
            auto r = area.removeFromTop (row);
            area.removeFromTop (gap);
            const int cells = numSwatches + 1;
            const int cell = r.getWidth() / cells;
            for (auto* sw : swatches)
                sw->setBounds (r.removeFromLeft (cell).reduced (1));
            moreColoursBtn.setBounds (r.reduced (1, 0));
        }
        {
            auto r = area.removeFromTop (row);
            area.removeFromTop (gap);
            // The label and the Dashed button take what their text needs (the
            // look-and-feel pads button text 7 px a side), the width selector
            // the rest, so "0.5 px" and the arrow always fit.
            const auto textWidth = [] (const juce::Font& f, const juce::String& s)
            {
                return juce::roundToInt (juce::GlyphArrangement::getStringWidth (f, s));
            };
            // Measured at the row's text size, not the Label's own font, which the
            // look-and-feel scales again when it draws.
            const juce::Font buttonFont (juce::FontOptions().withHeight (row * 0.6f));
            const int labelW = textWidth (buttonFont, widthLabel.getText())
                             + widthLabel.getBorderSize().getLeftAndRight() + 4;
            const int dashedW = juce::jmin (r.getWidth() / 2,
                                            textWidth (buttonFont, dashedBtn.getButtonText()) + 14 + 2 * gap);
            widthLabel.setBounds (r.removeFromLeft (labelW));
            dashedBtn.setBounds (r.removeFromRight (dashedW));
            r.removeFromRight (gap);
            widthCombo.setBounds (r);
        }
        labelEditor.setBounds (area.removeFromTop (row));
        area.removeFromTop (gap * 2);
        twoUp (copyBtn, &pasteBtn);
        deleteBtn.setBounds (area.removeFromTop (row));
    }

private:
    //==========================================================================
    struct Swatch : public juce::Button
    {
        explicit Swatch (juce::Colour c) : juce::Button ({}), colour (c) {}

        void setSelected (bool s) { if (selected != s) { selected = s; repaint(); } }

        void paintButton (juce::Graphics& g, bool over, bool down) override
        {
            auto b = getLocalBounds().toFloat().reduced (2.0f);
            const float d = juce::jmin (b.getWidth(), b.getHeight());
            auto circle = b.withSizeKeepingCentre (d, d);
            g.setColour (colour);
            g.fillEllipse (circle);
            g.setColour (selected ? ColorScheme::get().accentBlue
                                  : ColorScheme::get().buttonBorder.withAlpha (over || down ? 1.0f : 0.6f));
            g.drawEllipse (circle, selected ? 2.5f : 1.0f);
        }

        juce::Colour colour;
        bool selected = false;
    };

    static constexpr int numSwatches = 8;
    static constexpr juce::uint32 swatchColours[numSwatches] = {
        0xFFFFFFFF,  // white
        0xFFB0B0B0,  // grey (the default)
        0xFFFFC107,  // amber
        0xFFFF5252,  // red
        0xFF66BB6A,  // green
        0xFF26C6DA,  // cyan
        0xFF448AFF,  // blue
        0xFFE040FB   // magenta
    };
    static constexpr float widths[] = { 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f };

    int rowHeight() const { return juce::jmax (24, juce::roundToInt (28.0f * WfsLookAndFeel::uiScale)); }
    int gapSize() const   { return juce::jmax (3, juce::roundToInt (4.0f * WfsLookAndFeel::uiScale)); }
    int padSize() const   { return juce::jmax (6, juce::roundToInt (8.0f * WfsLookAndFeel::uiScale)); }

    void shapeClipboardChanged() override { refresh(); }

    void commitLabel()
    {
        if (updating || ! editor.hasSelection())
            return;
        if (labelEditor.getText() != editor.getDisplayedLabel())
            editor.setLabel (labelEditor.getText());
    }

    void setupHelpText()
    {
        helpText[&selectBtn]      = LOC ("map.shapes.help.select");
        helpText[&pointsBtn]      = LOC ("map.shapes.help.editPoints");
        helpText[&lineBtn]        = LOC ("map.shapes.help.line");
        helpText[&polygonBtn]     = LOC ("map.shapes.help.polygon");
        helpText[&rectBtn]        = LOC ("map.shapes.help.rectangle");
        helpText[&ellipseBtn]     = LOC ("map.shapes.help.ellipse");
        helpText[&freehandBtn]    = LOC ("map.shapes.help.freehand");
        helpText[&moreColoursBtn] = LOC ("map.shapes.help.colour");
        helpText[&widthCombo]     = LOC ("map.shapes.help.width");
        helpText[&dashedBtn]      = LOC ("map.shapes.help.dashed");
        helpText[&labelEditor]    = LOC ("map.shapes.help.label");
        helpText[&copyBtn]        = LOC ("map.shapes.help.copy");
        helpText[&pasteBtn]       = LOC ("map.shapes.help.paste");
        helpText[&deleteBtn]      = LOC ("map.shapes.help.delete");
        for (auto* sw : swatches)
            helpText[sw] = LOC ("map.shapes.help.colour");

        for (auto& [comp, text] : helpText)
            comp->addMouseListener (this, false);
    }

    void mouseEnter (const juce::MouseEvent& e) override
    {
        if (statusBar == nullptr)
            return;
        if (auto it = helpText.find (e.eventComponent); it != helpText.end())
            statusBar->setHelpText (it->second);
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (statusBar != nullptr)
            statusBar->clearText();
    }

    MapShapesEditor& editor;
    StatusBar* statusBar = nullptr;
    bool updating = false;

    juce::TextButton selectBtn, pointsBtn, lineBtn, polygonBtn, rectBtn, ellipseBtn, freehandBtn;
    juce::OwnedArray<Swatch> swatches;
    juce::TextButton moreColoursBtn;
    juce::Label widthLabel;
    juce::ComboBox widthCombo;
    juce::TextButton dashedBtn;
    juce::TextEditor labelEditor;
    juce::TextButton copyBtn, pasteBtn, deleteBtn;
    std::map<juce::Component*, juce::String> helpText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MapShapeEditPanel)
};
