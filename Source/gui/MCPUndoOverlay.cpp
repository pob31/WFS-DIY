#include "MCPUndoOverlay.h"
#include "../Localization/LocalizationManager.h"
#include "WfsLookAndFeel.h"

namespace
{
    juce::String formatElapsed (juce::Time created, juce::Time now)
    {
        const int seconds = static_cast<int> ((now - created).inSeconds());
        if (seconds < 60)
            return juce::String (seconds) + "s";
        const int minutes = seconds / 60;
        return juce::String (minutes) + "m " + juce::String (seconds - minutes * 60) + "s";
    }

    inline int scaled (int px) { return juce::roundToInt (static_cast<float> (px) * WfsLookAndFeel::uiScale); }
}

int MCPUndoOverlay::rowHeight()            { return scaled (38); }
int MCPUndoOverlay::headerHeight()         { return scaled (24); }
int MCPUndoOverlay::toastWidth()           { return scaled (380); }
int MCPUndoOverlay::paddingFromCorner()    { return scaled (12); }
int MCPUndoOverlay::rowDeleteButtonSize()  { return scaled (22); }

MCPUndoOverlay::MCPUndoOverlay (WFSNetwork::MCPUndoEngine& e,
                                WFSNetwork::MCPChangeRecordBuffer& b)
    : engine (e), buffer (b)
{
    ColorScheme::Manager::getInstance().addListener (this);
    setVisible (false);
    setInterceptsMouseClicks (true, false);
    setMouseCursor (juce::MouseCursor::NormalCursor);
    startTimer (200);  // 5 Hz poll — invisible latency, cheap
}

MCPUndoOverlay::~MCPUndoOverlay()
{
    stopTimer();
    ColorScheme::Manager::getInstance().removeListener (this);
}

void MCPUndoOverlay::positionInParent (juce::Rectangle<int> parentBounds)
{
    // Width fixed; height grows with row count, capped at kMaxVisibleRows.
    const int hH = headerHeight();
    const int rH = rowHeight();
    const int tW = toastWidth();
    const int pad = paddingFromCorner();

    const int visibleCount = juce::jmin (kMaxVisibleRows, static_cast<int> (rows.size()));
    const int extraRow = (static_cast<int> (rows.size()) > kMaxVisibleRows) ? hH : 0;  // "and N more"
    const int height = (visibleCount > 0)
                          ? (hH + visibleCount * rH + extraRow + scaled (8))
                          : 0;

    if (height == 0)
    {
        setVisible (false);
        return;
    }

    setBounds (parentBounds.getRight() - tW - pad,
               parentBounds.getY() + pad,
               tW,
               height);
    setVisible (true);
}

void MCPUndoOverlay::timerCallback()
{
    syncWithBuffer();

    // Drop rows that aged out (Block 3 will add a fade animation; for
    // Block 1 just remove instantly). Capture each aged-out timestamp so
    // syncWithBuffer doesn't immediately re-add the still-in-ring record.
    const auto now = juce::Time::getCurrentTime();
    rows.erase (std::remove_if (rows.begin(), rows.end(),
                                [&] (const DisplayedRow& r)
                                {
                                    if ((now - r.displayCreatedAt).inMilliseconds() > kRowLifetimeMs)
                                    {
                                        dismissedTimestamps.insert (r.record.timestamp.toMilliseconds());
                                        return true;
                                    }
                                    return false;
                                }),
                rows.end());

    if (auto* parent = getParentComponent())
        positionInParent (parent->getLocalBounds());

    repaint();
}

void MCPUndoOverlay::syncWithBuffer()
{
    // Snapshot the current ring (oldest first, newest last).
    const auto recent = buffer.getRecent (-1);

    // Build set of timestamps currently in the ring for fast membership.
    std::vector<juce::int64> currentTimestamps;
    currentTimestamps.reserve (recent.size());
    for (const auto& r : recent)
        currentTimestamps.push_back (r.timestamp.toMilliseconds());

    // Drop displayed rows whose backing record is gone (undone) or
    // self-corrected (operator overrode — toast hides per Phase 5b Block 4).
    rows.erase (std::remove_if (rows.begin(), rows.end(),
                                [&] (const DisplayedRow& dr)
                                {
                                    auto it = std::find (currentTimestamps.begin(),
                                                          currentTimestamps.end(),
                                                          dr.record.timestamp.toMilliseconds());
                                    if (it == currentTimestamps.end())
                                        return true;  // record gone
                                    // Find the latest copy and check the self-corrected flag.
                                    const auto idx = static_cast<size_t> (std::distance (currentTimestamps.begin(), it));
                                    return recent[idx].isSelfCorrected;
                                }),
                rows.end());

    // Prune dismissedTimestamps for records no longer in the ring (record
    // was undone or evicted) — keeps the set bounded.
    for (auto it = dismissedTimestamps.begin(); it != dismissedTimestamps.end(); )
    {
        if (std::find (currentTimestamps.begin(), currentTimestamps.end(), *it)
            == currentTimestamps.end())
            it = dismissedTimestamps.erase (it);
        else
            ++it;
    }

    // Add rows for new records (timestamps not in our displayed set, and
    // not previously dismissed by the user).
    const auto now = juce::Time::getCurrentTime();
    for (const auto& r : recent)
    {
        if (r.isSelfCorrected)
            continue;

        const auto ts = r.timestamp.toMilliseconds();
        if (dismissedTimestamps.count (ts) > 0)
            continue;

        const bool alreadyDisplayed =
            std::any_of (rows.begin(), rows.end(),
                         [ts] (const DisplayedRow& dr) { return dr.record.timestamp.toMilliseconds() == ts; });
        if (alreadyDisplayed)
            continue;

        DisplayedRow dr;
        dr.record = r;
        dr.displayCreatedAt = now;
        rows.push_back (std::move (dr));
    }

    // Newest at the top — sort by timestamp descending.
    std::sort (rows.begin(), rows.end(),
               [] (const DisplayedRow& a, const DisplayedRow& b)
               {
                   return a.record.timestamp.toMilliseconds() > b.record.timestamp.toMilliseconds();
               });
}

void MCPUndoOverlay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    auto& cs = ColorScheme::get();
    const float scale = WfsLookAndFeel::uiScale;

    const int hH = headerHeight();
    const int rH = rowHeight();
    const int delBtnSize = rowDeleteButtonSize();

    // Container — slightly translucent so underlying UI peeks through.
    g.setColour (cs.surfaceCard.withAlpha (0.96f));
    g.fillRoundedRectangle (bounds.toFloat(), scaled (6));
    g.setColour (cs.chromeDivider);
    g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f), scaled (6), 1.0f);

    // Header
    auto headerBounds = bounds.removeFromTop (hH);
    g.setColour (cs.textPrimary);
    g.setFont (juce::FontOptions (juce::jmax (10.0f, 12.0f * scale)).withStyle ("Bold"));
    g.drawText (LOC ("ai.toast.header"), headerBounds.reduced (scaled (8), 0),
                juce::Justification::centredLeft);

    // Close button (×) in top-right of the header
    const auto closeBtn = getCloseButtonBounds();
    g.setColour (hoveredCloseButton ? cs.accentRed : cs.textSecondary);
    g.setFont (juce::FontOptions (juce::jmax (12.0f, 16.0f * scale)));
    g.drawText (juce::CharPointer_UTF8 ("\xc3\x97"), closeBtn, juce::Justification::centred);

    // Header divider
    g.setColour (cs.chromeDivider);
    g.drawHorizontalLine (hH - 1,
                          static_cast<float> (bounds.getX()),
                          static_cast<float> (bounds.getRight()));

    // Rows
    const auto now = juce::Time::getCurrentTime();
    const int visibleCount = juce::jmin (kMaxVisibleRows, static_cast<int> (rows.size()));

    const auto clientPrefix = LOC ("ai.toast.clientPrefix");

    for (int i = 0; i < visibleCount; ++i)
    {
        auto rowBounds = getRowBounds (i);
        const auto& r = rows[static_cast<size_t> (i)];

        // Highlight the row when its × button is hover-previewed (or any
        // group-related row that the same × click would reverse).
        if (hoverPreviewRows.contains (i))
        {
            g.setColour (cs.accentRed.withAlpha (0.15f));
            g.fillRect (rowBounds);
        }
        else if (hoveredRow == i)
        {
            g.setColour (cs.buttonHover.withAlpha (0.4f));
            g.fillRect (rowBounds);
        }

        // Description (left-aligned, takes most of the row)
        auto textBounds = rowBounds.reduced (scaled (8), scaled (4));
        textBounds.removeFromRight (delBtnSize + scaled (4));

        // First line: client name + elapsed time (small, secondary)
        auto headerLineBounds = textBounds.removeFromTop (scaled (12));
        g.setColour (cs.textSecondary);
        g.setFont (juce::FontOptions (juce::jmax (9.0f, 10.0f * scale)));
        g.drawText (clientPrefix + juce::String (juce::CharPointer_UTF8 ("  \xe2\x80\xa2  "))
                        + formatElapsed (r.record.timestamp, now),
                    headerLineBounds, juce::Justification::centredLeft);

        // Second line: operator description (clipped to fit)
        g.setColour (cs.textPrimary);
        g.setFont (juce::FontOptions (juce::jmax (10.0f, 12.0f * scale)));
        g.drawText (r.record.operatorDescription, textBounds,
                    juce::Justification::centredLeft, true);

        // Per-row × button on the right
        const auto deleteBtn = getRowDeleteButtonBounds (i);
        const bool isHoveringDelete = (hoveredRow == i) && hoveredRowDeleteButton;
        g.setColour (isHoveringDelete ? cs.accentRed : cs.textSecondary);
        g.setFont (juce::FontOptions (juce::jmax (11.0f, 14.0f * scale)));
        g.drawText (juce::CharPointer_UTF8 ("\xc3\x97"), deleteBtn, juce::Justification::centred);
    }

    // "and N more" footer if rows overflow the cap
    if (static_cast<int> (rows.size()) > kMaxVisibleRows)
    {
        const int extra = static_cast<int> (rows.size()) - kMaxVisibleRows;
        auto footer = juce::Rectangle<int> (bounds.getX(),
                                            hH + visibleCount * rH,
                                            bounds.getWidth(), hH)
                         .reduced (scaled (8), scaled (2));
        g.setColour (cs.textSecondary);
        g.setFont (juce::FontOptions (juce::jmax (9.0f, 10.0f * scale)).withStyle ("Italic"));
        const auto moreText = LocalizationManager::getInstance().get (
            "ai.toast.moreOlder", {{"count", juce::String (extra)}});
        g.drawText (moreText, footer, juce::Justification::centredLeft);
    }
}

void MCPUndoOverlay::resized()
{
    // No child components to lay out — paint() draws everything.
}

void MCPUndoOverlay::mouseDown (const juce::MouseEvent& event)
{
    // Container close button hides the toast for this round; it'll
    // re-show when a new AI record arrives. Capture each visible row's
    // timestamp so syncWithBuffer doesn't re-add them on the next tick.
    if (getCloseButtonBounds().contains (event.getPosition()))
    {
        for (const auto& r : rows)
            dismissedTimestamps.insert (r.record.timestamp.toMilliseconds());
        rows.clear();
        if (auto* parent = getParentComponent())
            positionInParent (parent->getLocalBounds());
        return;
    }

    // Per-row × → targeted undo via the engine.
    const int rowIdx = rowIndexAtY (event.y);
    if (rowIdx >= 0 && getRowDeleteButtonBounds (rowIdx).contains (event.getPosition()))
    {
        const int bufIdx = bufferIndexForRow (rowIdx);
        if (bufIdx >= 0)
        {
            // Tag the call as MCP-origin so the engine's listener doesn't
            // notify on its own undo writes.
            WFSNetwork::OriginTagScope originScope { WFSNetwork::OriginTag::MCP };
            engine.undoByIndex (bufIdx);
        }
        // Resync immediately so the row(s) disappear without waiting
        // for the next timer tick.
        syncWithBuffer();
        if (auto* parent = getParentComponent())
            positionInParent (parent->getLocalBounds());
        repaint();
    }
}

void MCPUndoOverlay::mouseMove (const juce::MouseEvent& event)
{
    const auto pos = event.getPosition();

    const bool nowHoveringClose = getCloseButtonBounds().contains (pos);
    const int newHoveredRow = rowIndexAtY (event.y);
    const bool newHoveredDelete =
        (newHoveredRow >= 0) && getRowDeleteButtonBounds (newHoveredRow).contains (pos);

    if (nowHoveringClose != hoveredCloseButton
        || newHoveredRow != hoveredRow
        || newHoveredDelete != hoveredRowDeleteButton)
    {
        hoveredCloseButton = nowHoveringClose;
        hoveredRow = newHoveredRow;
        hoveredRowDeleteButton = newHoveredDelete;
        hoverPreviewRows.clearQuick();
        if (newHoveredDelete && newHoveredRow >= 0)
            hoverPreviewRows = previewForRow (newHoveredRow);
        repaint();
    }
}

void MCPUndoOverlay::mouseExit (const juce::MouseEvent&)
{
    if (hoveredRow >= 0 || hoveredRowDeleteButton || hoveredCloseButton || ! hoverPreviewRows.isEmpty())
    {
        hoveredRow = -1;
        hoveredRowDeleteButton = false;
        hoveredCloseButton = false;
        hoverPreviewRows.clearQuick();
        repaint();
    }
}

int MCPUndoOverlay::rowIndexAtY (int y) const
{
    const int hH = headerHeight();
    const int rH = rowHeight();
    if (y < hH) return -1;
    const int idx = (y - hH) / rH;
    if (idx < 0 || idx >= static_cast<int> (rows.size())) return -1;
    if (idx >= kMaxVisibleRows) return -1;
    return idx;
}

juce::Array<int> MCPUndoOverlay::previewForRow (int rowIdx) const
{
    juce::Array<int> result;
    if (rowIdx < 0 || rowIdx >= static_cast<int> (rows.size()))
        return result;

    const int bufIdx = bufferIndexForRow (rowIdx);
    if (bufIdx < 0)
        return result;

    const auto bufferIndices = engine.previewUndo (bufIdx);

    // Translate buffer indices back to row indices via timestamp matching.
    for (const int bufferTargetIdx : bufferIndices)
    {
        WFSNetwork::ChangeRecord target;
        if (! buffer.peekAt (bufferTargetIdx, target))
            continue;
        for (size_t r = 0; r < rows.size(); ++r)
        {
            if (rows[r].record.timestamp.toMilliseconds() == target.timestamp.toMilliseconds())
            {
                result.add (static_cast<int> (r));
                break;
            }
        }
    }

    return result;
}

int MCPUndoOverlay::bufferIndexForRow (int rowIdx) const
{
    if (rowIdx < 0 || rowIdx >= static_cast<int> (rows.size()))
        return -1;

    // Find the buffer index whose record has the same timestamp as the row.
    const auto target = rows[static_cast<size_t> (rowIdx)].record.timestamp.toMilliseconds();
    const int total = buffer.size();
    for (int i = 0; i < total; ++i)
    {
        WFSNetwork::ChangeRecord r;
        if (buffer.peekAt (i, r) && r.timestamp.toMilliseconds() == target)
            return i;
    }
    return -1;
}

juce::Rectangle<int> MCPUndoOverlay::getCloseButtonBounds() const
{
    const int hH = headerHeight();
    return juce::Rectangle<int> (getWidth() - hH, 0, hH, hH);
}

juce::Rectangle<int> MCPUndoOverlay::getRowBounds (int rowIdx) const
{
    return juce::Rectangle<int> (0,
                                  headerHeight() + rowIdx * rowHeight(),
                                  getWidth(),
                                  rowHeight());
}

juce::Rectangle<int> MCPUndoOverlay::getRowDeleteButtonBounds (int rowIdx) const
{
    auto row = getRowBounds (rowIdx);
    const int btnSize = rowDeleteButtonSize();
    const int x = row.getRight() - btnSize - scaled (4);
    const int y = row.getY() + (row.getHeight() - btnSize) / 2;
    return juce::Rectangle<int> (x, y, btnSize, btnSize);
}
