#include "MCPHistoryWindow.h"
#include "../Network/OSCProtocolTypes.h"
#include "../Localization/LocalizationManager.h"
#include "WfsLookAndFeel.h"

namespace
{
    juce::String formatTimestamp (juce::Time t)
    {
        return t.formatted ("%H:%M:%S");
    }

    // Layout constants are baseline values; everything is multiplied by
    // WfsLookAndFeel::uiScale at use-site so the window scales with the
    // rest of the app's UI.
    inline int scaled (int px) { return juce::roundToInt (static_cast<float> (px) * WfsLookAndFeel::uiScale); }

    inline int headerHeight()    { return scaled (36); }
    inline int footerHeight()    { return scaled (44); }
    inline int cursorThickness() { return scaled (6); }
    inline int actionButtonSize(){ return scaled (22); }
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
        stepBackButton.setButtonText (LOC ("ai.history.stepBack"));
        stepBackButton.onClick = [this] { stepBack(); };

        addAndMakeVisible (stepForwardButton);
        stepForwardButton.setButtonText (LOC ("ai.history.stepForward"));
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

        const int headerH = headerHeight();
        const int footerH = footerHeight();

        // Header strip
        auto headerBounds = getLocalBounds().removeFromTop (headerH);
        g.setColour (cs.chromeBackground);
        g.fillRect (headerBounds);
        g.setColour (cs.textPrimary);
        g.setFont (juce::FontOptions (juce::jmax (12.0f, 15.0f * WfsLookAndFeel::uiScale)).withStyle ("Bold"));
        g.drawText (LOC ("ai.history.windowTitle"),
                    headerBounds.reduced (scaled (12), 0),
                    juce::Justification::centredLeft);
        g.setColour (cs.chromeDivider);
        g.drawHorizontalLine (headerH - 1,
                              static_cast<float> (headerBounds.getX()),
                              static_cast<float> (headerBounds.getRight()));

        // Footer strip
        auto footerBounds = getLocalBounds().removeFromBottom (footerH);
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
        bounds.removeFromTop (headerHeight());
        auto footer = bounds.removeFromBottom (footerHeight()).reduced (scaled (12), scaled (6));

        // Resize the inner table to viewport width so wide rows wrap correctly.
        const int contentWidth = bounds.getWidth() - viewport.getScrollBarThickness();
        const int contentHeight = juce::jmax (bounds.getHeight(),
                                              headerHeight() + table.getNumRows() * MCPHistoryTableComponent::rowHeight() + cursorThickness());
        table.setSize (contentWidth, contentHeight);
        viewport.setBounds (bounds);

        // Footer layout: [Step Back][gap][position label][count][gap][Step Forward]
        auto stepBackBounds    = footer.removeFromLeft  (scaled (110));
        auto stepForwardBounds = footer.removeFromRight (scaled (110));
        auto countBounds       = footer.removeFromRight (scaled (60));
        stepBackButton.setBounds (stepBackBounds.reduced (0, scaled (4)));
        stepForwardButton.setBounds (stepForwardBounds.reduced (0, scaled (4)));
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
        const int needed = total * MCPHistoryTableComponent::rowHeight() + cursorThickness();
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
        const auto applied = LOC ("ai.history.applied");
        const auto undone  = LOC ("ai.history.undone");
        const auto ofWord  = LOC ("ai.history.of");
        positionLabel.setText (juce::String (undoable) + " " + applied
                                + " / " + juce::String (total - undoable) + " " + undone,
                                juce::dontSendNotification);
        countLabel.setText (juce::String (total) + " " + ofWord + " "
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
int MCPHistoryTableComponent::rowHeight()
{
    return juce::roundToInt (28.0f * WfsLookAndFeel::uiScale);
}

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

    const int rh = rowHeight();
    const int ct = cursorThickness();

    int yCursor = undoableCount * rh;
    if (y < yCursor)
        return y / rh;

    if (y < yCursor + ct)
        return -1;  // hit the divider, ignore

    const int adjustedY = y - ct;
    const int idx = adjustedY / rh;
    if (idx < 0 || idx >= static_cast<int> (rows.size()))
        return -1;
    return idx;
}

juce::Rectangle<int> MCPHistoryTableComponent::actionButtonBoundsForRow (int rowIdx) const
{
    if (rowIdx < 0 || rowIdx >= static_cast<int> (rows.size()))
        return {};

    const int rh = rowHeight();
    const int ct = cursorThickness();
    const int btnSize = actionButtonSize();

    const int rowTop = (rowIdx < undoableCount)
                          ? (rowIdx * rh)
                          : (undoableCount * rh + ct
                              + (rowIdx - undoableCount) * rh);
    const int x = getWidth() - btnSize - scaled (8);
    const int y = rowTop + (rh - btnSize) / 2;
    return juce::Rectangle<int> (x, y, btnSize, btnSize);
}

void MCPHistoryTableComponent::paint (juce::Graphics& g)
{
    auto& cs = ColorScheme::get();
    g.fillAll (cs.background);

    const float scale = WfsLookAndFeel::uiScale;

    if (rows.empty())
    {
        g.setColour (cs.textSecondary);
        g.setFont (juce::FontOptions (juce::jmax (10.0f, 13.0f * scale)));
        g.drawText (LOC ("ai.history.noChanges"), getLocalBounds(), juce::Justification::centred);
        return;
    }

    const int rh        = rowHeight();
    const int ct        = cursorThickness();
    const int btnSize   = actionButtonSize();
    const int width     = getWidth();
    int y = 0;

    for (int i = 0; i < static_cast<int> (rows.size()); ++i)
    {
        // Insert the cursor divider between the last applied row and the
        // first undone row.
        if (i == undoableCount)
        {
            g.setColour (cs.accentBlue.withAlpha (0.35f));
            g.fillRect (0, y, width, ct);
            g.setColour (cs.accentBlue);
            g.setFont (juce::FontOptions (juce::jmax (9.0f, 10.0f * scale)).withStyle ("Bold"));
            g.drawText (LOC ("ai.history.cursorLabel"),
                        scaled (12), y - 1, width - scaled (24), ct + 2,
                        juce::Justification::centredLeft);
            y += ct;
        }

        const auto& row = rows[static_cast<size_t> (i)];
        const bool isHovered = (hoveredRow == i);
        juce::Rectangle<int> rowBounds (0, y, width, rh);

        // Row background
        if (isHovered)
        {
            g.setColour (cs.buttonHover.withAlpha (0.4f));
            g.fillRect (rowBounds);
        }

        const float baseAlpha = (row.isApplied ? 1.0f : 0.7f);
        const float alpha = row.record.isSelfCorrected ? 0.4f : baseAlpha;

        auto cell = rowBounds.reduced (scaled (8), scaled (4));

        // Timestamp
        auto tsBounds = cell.removeFromLeft (scaled (62));
        g.setColour (cs.textSecondary.withAlpha (alpha));
        g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.0f * scale)));
        g.drawText (formatTimestamp (row.record.timestamp), tsBounds,
                    juce::Justification::centredLeft);

        // Tool name (mono, dim)
        auto toolBounds = cell.removeFromLeft (scaled (160));
        g.setColour (cs.textSecondary.withAlpha (alpha));
        g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(),
                                      juce::jmax (9.0f, 11.0f * scale), juce::Font::plain));
        g.drawText (row.record.toolName,
                    toolBounds,
                    juce::Justification::centredLeft, true);

        // Action button column reserved on the right
        cell.removeFromRight (btnSize + scaled (8));

        // Batch tag if applicable
        if (row.record.redoBatchId != 0)
        {
            auto batchBounds = cell.removeFromRight (scaled (60));
            g.setColour (cs.accentGreen.withAlpha (alpha * 0.7f));
            g.setFont (juce::FontOptions (juce::jmax (9.0f, 10.0f * scale)).withStyle ("Bold"));
            const auto batchText = LocalizationManager::getInstance().get (
                "ai.history.batch", {{"id", juce::String (row.record.redoBatchId)}});
            g.drawText (batchText, batchBounds, juce::Justification::centredRight);
        }

        // Status badge
        auto statusBounds = cell.removeFromRight (scaled (24));
        g.setFont (juce::FontOptions (juce::jmax (10.0f, 13.0f * scale)));
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
        g.setFont (juce::FontOptions (juce::jmax (10.0f, 12.0f * scale)));
        g.drawText (row.record.operatorDescription, cell,
                    juce::Justification::centredLeft, true);

        // Action button on the right edge
        const auto btnBounds = actionButtonBoundsForRow (i);
        const bool showAction = row.isApplied
                                  || (! row.isApplied && i == undoableCount);  // topmost undone
        if (showAction)
        {
            const bool actionHover = (isHovered && hoveredAction);
            g.setFont (juce::FontOptions (juce::jmax (12.0f, 16.0f * scale)));
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

        y += rh;
    }

    // Bottom cursor (if there are no undone rows, draw an "all caught up" hint)
    if (undoableCount == static_cast<int> (rows.size()) && ! rows.empty())
    {
        g.setColour (cs.accentBlue.withAlpha (0.25f));
        g.fillRect (0, y, width, ct);
        g.setColour (cs.textSecondary);
        g.setFont (juce::FontOptions (juce::jmax (9.0f, 10.0f * scale)).withStyle ("Italic"));
        g.drawText (LOC ("ai.history.atHead"),
                    scaled (12), y + ct + 2, width - scaled (24), scaled (14),
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
    : juce::DocumentWindow (LOC ("ai.history.windowTitle"),
                            ColorScheme::get().background,
                            juce::DocumentWindow::allButtons)
{
    setUsingNativeTitleBar (true);
    setResizable (true, true);

    auto* content = new MCPHistoryWindowContent (engine, undoRing);
    setContentOwned (content, false);

    const int w = juce::roundToInt (760.0f * WfsLookAndFeel::uiScale);
    const int h = juce::roundToInt (480.0f * WfsLookAndFeel::uiScale);
    setSize (w, h);
    centreWithSize (w, h);

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
