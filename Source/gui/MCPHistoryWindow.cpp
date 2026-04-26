#include "MCPHistoryWindow.h"
#include "../Network/OSCProtocolTypes.h"

namespace
{
    juce::String formatTimestamp (juce::Time t)
    {
        return t.formatted ("%H:%M:%S");
    }

    constexpr int kHeaderHeight = 36;
    constexpr int kFooterHeight = 44;
    constexpr int kCursorThickness = 6;
    constexpr int kActionButtonSize = 22;
}

//==============================================================================
// MCPHistoryWindowContent — wraps the table in a Viewport, owns header + footer.
//==============================================================================
class MCPHistoryWindowContent : public juce::Component,
                                  public ColorScheme::Manager::Listener,
                                  private juce::Timer
{
public:
    MCPHistoryWindowContent (WFSNetwork::MCPUndoEngine& e,
                             WFSNetwork::MCPChangeRecordBuffer& undoRing)
        : engine (e), table (e, undoRing)
    {
        ColorScheme::Manager::getInstance().addListener (this);

        viewport.setViewedComponent (&table, false);
        viewport.setScrollBarsShown (true, false);
        addAndMakeVisible (viewport);

        addAndMakeVisible (stepBackButton);
        stepBackButton.setButtonText (juce::CharPointer_UTF8 ("\xe2\x8f\xae Step Back"));
        stepBackButton.onClick = [this] { stepBack(); };

        addAndMakeVisible (stepForwardButton);
        stepForwardButton.setButtonText (juce::CharPointer_UTF8 ("Step Forward \xe2\x8f\xad"));
        stepForwardButton.onClick = [this] { stepForward(); };

        addAndMakeVisible (positionLabel);
        positionLabel.setJustificationType (juce::Justification::centred);

        addAndMakeVisible (countLabel);
        countLabel.setJustificationType (juce::Justification::centredRight);

        startTimerHz (4);  // refresh footer labels at 4 Hz; the table polls at 2 Hz
        applyColors();
        updateFooterLabels();
    }

    ~MCPHistoryWindowContent() override
    {
        ColorScheme::Manager::getInstance().removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        auto& cs = ColorScheme::get();
        g.fillAll (cs.background);

        // Header strip
        auto headerBounds = getLocalBounds().removeFromTop (kHeaderHeight);
        g.setColour (cs.chromeBackground);
        g.fillRect (headerBounds);
        g.setColour (cs.textPrimary);
        g.setFont (juce::FontOptions (15.0f).withStyle ("Bold"));
        g.drawText ("AI Change History",
                    headerBounds.reduced (12, 0),
                    juce::Justification::centredLeft);
        g.setColour (cs.chromeDivider);
        g.drawHorizontalLine (kHeaderHeight - 1,
                              static_cast<float> (headerBounds.getX()),
                              static_cast<float> (headerBounds.getRight()));

        // Footer strip
        auto footerBounds = getLocalBounds().removeFromBottom (kFooterHeight);
        g.setColour (cs.chromeSurface);
        g.fillRect (footerBounds);
        g.setColour (cs.chromeDivider);
        g.drawHorizontalLine (footerBounds.getY(),
                              static_cast<float> (footerBounds.getX()),
                              static_cast<float> (footerBounds.getRight()));
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop (kHeaderHeight);
        auto footer = bounds.removeFromBottom (kFooterHeight).reduced (12, 6);

        // Resize the inner table to viewport width so wide rows wrap correctly.
        const int contentWidth = bounds.getWidth() - viewport.getScrollBarThickness();
        const int contentHeight = juce::jmax (bounds.getHeight(),
                                              kHeaderHeight + table.getNumRows() * MCPHistoryTableComponent::kRowHeight + kCursorThickness);
        table.setSize (contentWidth, contentHeight);
        viewport.setBounds (bounds);

        // Footer layout: [Step Back][gap][position label][count][gap][Step Forward]
        auto stepBackBounds    = footer.removeFromLeft (110);
        auto stepForwardBounds = footer.removeFromRight (110);
        auto countBounds       = footer.removeFromRight (60);
        stepBackButton.setBounds (stepBackBounds.reduced (0, 4));
        stepForwardButton.setBounds (stepForwardBounds.reduced (0, 4));
        countLabel.setBounds (countBounds);
        positionLabel.setBounds (footer);
    }

    void colorSchemeChanged() override
    {
        applyColors();
        repaint();
        table.repaint();
    }

private:
    void applyColors()
    {
        auto& cs = ColorScheme::get();
        positionLabel.setColour (juce::Label::textColourId, cs.textPrimary);
        countLabel.setColour    (juce::Label::textColourId, cs.textSecondary);
        stepBackButton.setColour    (juce::TextButton::buttonColourId, cs.buttonNormal);
        stepBackButton.setColour    (juce::TextButton::textColourOffId, cs.textPrimary);
        stepForwardButton.setColour (juce::TextButton::buttonColourId, cs.buttonNormal);
        stepForwardButton.setColour (juce::TextButton::textColourOffId, cs.textPrimary);
    }

    void timerCallback() override
    {
        updateFooterLabels();

        // Resize the table to its current row count if it changed.
        const int total = table.getNumRows();
        const int needed = total * MCPHistoryTableComponent::kRowHeight + kCursorThickness;
        if (table.getHeight() < needed)
        {
            const int contentWidth = juce::jmax (1, getWidth() - viewport.getScrollBarThickness());
            table.setSize (contentWidth, juce::jmax (viewport.getHeight(), needed));
        }
    }

    void updateFooterLabels()
    {
        const int undoable = table.getUndoableCount();
        const int total    = table.getNumRows();
        positionLabel.setText (juce::String (undoable) + " applied / "
                                + juce::String (total - undoable) + " undone",
                                juce::dontSendNotification);
        countLabel.setText (juce::String (total) + " of "
                              + juce::String (WFSNetwork::MCPChangeRecordBuffer::kDefaultCapacity),
                              juce::dontSendNotification);

        stepBackButton.setEnabled (undoable > 0);
        stepForwardButton.setEnabled (total - undoable > 0);
    }

    void stepBack()
    {
        WFSNetwork::OriginTagScope originScope { WFSNetwork::OriginTag::MCP };
        engine.undoLast();
        table.refreshNow();
        updateFooterLabels();
    }

    void stepForward()
    {
        WFSNetwork::OriginTagScope originScope { WFSNetwork::OriginTag::MCP };
        engine.redoLast();
        table.refreshNow();
        updateFooterLabels();
    }

    WFSNetwork::MCPUndoEngine& engine;
    juce::Viewport viewport;
    MCPHistoryTableComponent table;
    juce::Label positionLabel;
    juce::Label countLabel;
    juce::TextButton stepBackButton;
    juce::TextButton stepForwardButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPHistoryWindowContent)
};

//==============================================================================
// MCPHistoryTableComponent
//==============================================================================
MCPHistoryTableComponent::MCPHistoryTableComponent (WFSNetwork::MCPUndoEngine& e,
                                                      WFSNetwork::MCPChangeRecordBuffer& ring)
    : engine (e), undoRing (ring)
{
    setMouseCursor (juce::MouseCursor::NormalCursor);
    setInterceptsMouseClicks (true, false);
    rebuildRows();
    startTimer (500);  // 2 Hz
}

MCPHistoryTableComponent::~MCPHistoryTableComponent()
{
    stopTimer();
}

void MCPHistoryTableComponent::refreshNow()
{
    rebuildRows();
    repaint();
    if (auto* parent = getParentComponent())
        parent->repaint();  // refresh footer labels
}

void MCPHistoryTableComponent::timerCallback()
{
    rebuildRows();
    repaint();
}

void MCPHistoryTableComponent::rebuildRows()
{
    rows.clear();

    // Applied side (newest first → top of the display).
    auto applied = undoRing.getRecent (-1);  // oldest first, newest last
    for (int i = static_cast<int> (applied.size()) - 1; i >= 0; --i)
    {
        DisplayRow dr;
        dr.record = applied[static_cast<size_t> (i)];
        dr.isApplied = true;
        dr.bufferIndex = i;  // index into the undoRing snapshot we just took
        rows.push_back (std::move (dr));
    }

    undoableCount = static_cast<int> (rows.size());

    // Undone side: redo ring oldest-first → newest of redo ring is the
    // next-to-redo. Render the next-to-redo immediately below the cursor,
    // older redoable records below that.
    auto redoable = engine.getRedoStackSnapshot();
    for (int i = static_cast<int> (redoable.size()) - 1; i >= 0; --i)
    {
        DisplayRow dr;
        dr.record = redoable[static_cast<size_t> (i)];
        dr.isApplied = false;
        dr.bufferIndex = i;  // index into the redo snapshot
        rows.push_back (std::move (dr));
    }
}

int MCPHistoryTableComponent::rowIndexAtY (int y) const
{
    // Account for the cursor divider that sits between applied and undone
    // halves: rows above the divider use the natural index; rows below
    // need to add the divider thickness.
    if (y < 0) return -1;

    int yCursor = undoableCount * kRowHeight;
    if (y < yCursor)
        return y / kRowHeight;

    if (y < yCursor + kCursorThickness)
        return -1;  // hit the divider, ignore

    const int adjustedY = y - kCursorThickness;
    const int idx = adjustedY / kRowHeight;
    if (idx < 0 || idx >= static_cast<int> (rows.size()))
        return -1;
    return idx;
}

juce::Rectangle<int> MCPHistoryTableComponent::actionButtonBoundsForRow (int rowIdx) const
{
    if (rowIdx < 0 || rowIdx >= static_cast<int> (rows.size()))
        return {};

    const int rowTop = (rowIdx < undoableCount)
                          ? (rowIdx * kRowHeight)
                          : (undoableCount * kRowHeight + kCursorThickness
                              + (rowIdx - undoableCount) * kRowHeight);
    const int x = getWidth() - kActionButtonSize - 8;
    const int y = rowTop + (kRowHeight - kActionButtonSize) / 2;
    return juce::Rectangle<int> (x, y, kActionButtonSize, kActionButtonSize);
}

void MCPHistoryTableComponent::paint (juce::Graphics& g)
{
    auto& cs = ColorScheme::get();
    g.fillAll (cs.background);

    if (rows.empty())
    {
        g.setColour (cs.textSecondary);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText ("No AI changes yet.", getLocalBounds(), juce::Justification::centred);
        return;
    }

    const int width = getWidth();
    int y = 0;

    for (int i = 0; i < static_cast<int> (rows.size()); ++i)
    {
        // Insert the cursor divider between the last applied row and the
        // first undone row.
        if (i == undoableCount)
        {
            g.setColour (cs.accentBlue.withAlpha (0.35f));
            g.fillRect (0, y, width, kCursorThickness);
            g.setColour (cs.accentBlue);
            g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
            g.drawText (juce::CharPointer_UTF8 ("\xe2\x97\x82  cursor (\xe2\x86\x91 applied  /  \xe2\x86\x93 undone, redoable)"),
                        12, y - 1, width - 24, kCursorThickness + 2,
                        juce::Justification::centredLeft);
            y += kCursorThickness;
        }

        const auto& row = rows[static_cast<size_t> (i)];
        const bool isHovered = (hoveredRow == i);
        juce::Rectangle<int> rowBounds (0, y, width, kRowHeight);

        // Row background
        if (isHovered)
        {
            g.setColour (cs.buttonHover.withAlpha (0.4f));
            g.fillRect (rowBounds);
        }

        const float baseAlpha = (row.isApplied ? 1.0f : 0.7f);
        const float alpha = row.record.isSelfCorrected ? 0.4f : baseAlpha;

        auto cell = rowBounds.reduced (8, 4);

        // Timestamp
        auto tsBounds = cell.removeFromLeft (62);
        g.setColour (cs.textSecondary.withAlpha (alpha));
        g.setFont (juce::FontOptions (11.0f));
        g.drawText (formatTimestamp (row.record.timestamp), tsBounds,
                    juce::Justification::centredLeft);

        // Tool name (mono, dim)
        auto toolBounds = cell.removeFromLeft (160);
        g.setColour (cs.textSecondary.withAlpha (alpha));
        g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
        g.drawText (row.record.toolName.isNotEmpty() ? row.record.toolName : juce::String ("(unknown)"),
                    toolBounds,
                    juce::Justification::centredLeft, true);

        // Action button column reserved on the right
        cell.removeFromRight (kActionButtonSize + 8);

        // Batch tag if applicable
        if (row.record.redoBatchId != 0)
        {
            auto batchBounds = cell.removeFromRight (60);
            g.setColour (cs.accentGreen.withAlpha (alpha * 0.7f));
            g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
            g.drawText ("batch " + juce::String (row.record.redoBatchId), batchBounds,
                        juce::Justification::centredRight);
        }

        // Status badge
        auto statusBounds = cell.removeFromRight (24);
        g.setFont (juce::FontOptions (13.0f));
        if (row.record.isSelfCorrected)
        {
            g.setColour (cs.accentRed.withAlpha (alpha));
            g.drawText (juce::CharPointer_UTF8 ("\xe2\x8a\x98"), statusBounds,
                        juce::Justification::centred);
        }
        else if (row.isApplied)
        {
            g.setColour (cs.accentGreen.withAlpha (alpha));
            g.drawText (juce::CharPointer_UTF8 ("\xe2\x9c\x93"), statusBounds,
                        juce::Justification::centred);
        }
        else
        {
            g.setColour (cs.accentBlue.withAlpha (alpha));
            g.drawText (juce::CharPointer_UTF8 ("\xe2\x86\xb6"), statusBounds,
                        juce::Justification::centred);
        }

        // Description (the rest of the row)
        g.setColour (cs.textPrimary.withAlpha (alpha));
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (row.record.operatorDescription, cell,
                    juce::Justification::centredLeft, true);

        // Action button on the right edge
        const auto btnBounds = actionButtonBoundsForRow (i);
        const bool showAction = row.isApplied
                                  || (! row.isApplied && i == undoableCount);  // topmost undone
        if (showAction)
        {
            const bool actionHover = (isHovered && hoveredAction);
            g.setFont (juce::FontOptions (16.0f));
            if (row.isApplied)
            {
                g.setColour (actionHover ? cs.accentRed : cs.textSecondary);
                g.drawText (juce::CharPointer_UTF8 ("\xc3\x97"),  // ×
                            btnBounds, juce::Justification::centred);
            }
            else
            {
                g.setColour (actionHover ? cs.accentGreen : cs.textSecondary);
                g.drawText (juce::CharPointer_UTF8 ("\xe2\x86\xba"),  // ↺
                            btnBounds, juce::Justification::centred);
            }
        }

        y += kRowHeight;
    }

    // Bottom cursor (if there are no undone rows, draw an "all caught up" hint)
    if (undoableCount == static_cast<int> (rows.size()) && ! rows.empty())
    {
        g.setColour (cs.accentBlue.withAlpha (0.25f));
        g.fillRect (0, y, width, kCursorThickness);
        g.setColour (cs.textSecondary);
        g.setFont (juce::FontOptions (10.0f).withStyle ("Italic"));
        g.drawText ("(no undone records — at the head)",
                    12, y + kCursorThickness + 2, width - 24, 14,
                    juce::Justification::centredLeft);
    }
}

void MCPHistoryTableComponent::mouseMove (const juce::MouseEvent& event)
{
    const int newHover = rowIndexAtY (event.y);
    bool newHoverAction = false;
    if (newHover >= 0)
        newHoverAction = actionButtonBoundsForRow (newHover).contains (event.getPosition());

    if (newHover != hoveredRow || newHoverAction != hoveredAction)
    {
        hoveredRow = newHover;
        hoveredAction = newHoverAction;
        repaint();
    }
}

void MCPHistoryTableComponent::mouseExit (const juce::MouseEvent&)
{
    if (hoveredRow != -1 || hoveredAction)
    {
        hoveredRow = -1;
        hoveredAction = false;
        repaint();
    }
}

void MCPHistoryTableComponent::mouseDown (const juce::MouseEvent& event)
{
    const int rowIdx = rowIndexAtY (event.y);
    if (rowIdx < 0 || rowIdx >= static_cast<int> (rows.size()))
        return;

    if (! actionButtonBoundsForRow (rowIdx).contains (event.getPosition()))
        return;

    const auto& row = rows[static_cast<size_t> (rowIdx)];

    WFSNetwork::OriginTagScope originScope { WFSNetwork::OriginTag::MCP };
    if (row.isApplied)
    {
        // × — targeted undo. Map our row index back to the undoRing index
        // (oldest=0, newest=size-1). rebuildRows put the newest undo entry
        // at row 0, so undoRing index = (undoableCount - 1) - rowIdx.
        const int bufIdx = (undoableCount - 1) - rowIdx;
        engine.undoByIndex (bufIdx);
    }
    else
    {
        // ↺ — only the topmost-undone row (rowIdx == undoableCount) shows
        // the button; treat the click as Step Forward.
        if (rowIdx == undoableCount)
            engine.redoLast();
    }
    refreshNow();
}

//==============================================================================
// MCPHistoryWindow
//==============================================================================
MCPHistoryWindow::MCPHistoryWindow (WFSNetwork::MCPUndoEngine& engine,
                                    WFSNetwork::MCPChangeRecordBuffer& undoRing)
    : juce::DocumentWindow ("AI Change History",
                            ColorScheme::get().background,
                            juce::DocumentWindow::allButtons)
{
    setUsingNativeTitleBar (true);
    setResizable (true, true);

    auto* content = new MCPHistoryWindowContent (engine, undoRing);
    setContentOwned (content, false);

    setSize (760, 480);
    centreWithSize (760, 480);

    ColorScheme::Manager::getInstance().addListener (this);
}

MCPHistoryWindow::~MCPHistoryWindow()
{
    ColorScheme::Manager::getInstance().removeListener (this);
}

void MCPHistoryWindow::closeButtonPressed()
{
    setVisible (false);
}

void MCPHistoryWindow::colorSchemeChanged()
{
    setBackgroundColour (ColorScheme::get().background);
    repaint();
}
