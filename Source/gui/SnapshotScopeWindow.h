#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSFileManager.h"
#include "../Parameters/ParameterDirtyTracker.h"
#include "../Localization/LocalizationManager.h"
#include "ColorScheme.h"
#include "WfsLookAndFeel.h"
#include "WindowUtils.h"

/**
 * Snapshot Scope Window
 * Allows editing the scope for input snapshots with parameter-level, per-channel control.
 * Parameters are grouped into scope items and organized by sections.
 */

//==============================================================================
// Scope Grid Component - Handles the scrollable grid of cells
//==============================================================================
class ScopeGridComponent : public juce::Component
{
public:
    using ExtendedScope = WFSFileManager::ExtendedSnapshotScope;
    using ScopeItem = WFSFileManager::ScopeItem;
    using InclusionState = ExtendedScope::InclusionState;

    ScopeGridComponent (ExtendedScope& scopeRef, int numChannelsValue,
                        const ParameterDirtyTracker* dirtyTrackerPtr = nullptr)
        : scope (scopeRef), numChannels (numChannelsValue), dirtyTracker (dirtyTrackerPtr)
    {
        buildLayout();
    }

    void buildLayout()
    {
        visibleRows.clear();
        sectionStartRows.clear();

        const auto& sections = ExtendedScope::getSectionIds();
        int currentRow = 0;

        for (const auto& sectionId : sections)
        {
            // Add section header row
            sectionStartRows[sectionId.toString()] = currentRow;
            visibleRows.push_back ({ sectionId.toString(), true, true });  // isSection = true
            ++currentRow;

            // Add items for this section if expanded
            if (expandedSections.find (sectionId.toString()) == expandedSections.end())
                expandedSections[sectionId.toString()] = true;  // Default: expanded

            if (expandedSections[sectionId.toString()])
            {
                for (const auto* item : ExtendedScope::getItemsForSection (sectionId))
                {
                    visibleRows.push_back ({ item->itemId, false, false });
                    ++currentRow;
                }
            }
        }

        updateSize();
    }

    void updateSize()
    {
        int width = paramLabelWidth + numChannels * cellSize;
        int height = static_cast<int> (visibleRows.size()) * cellSize;
        setSize (width, height);
    }

    void paint (juce::Graphics& g) override
    {
        const auto& colors = ColorScheme::get();

        // Draw grid background
        g.fillAll (colors.surfaceCard);

        // Draw cells
        for (int row = 0; row < static_cast<int> (visibleRows.size()); ++row)
        {
            const auto& rowInfo = visibleRows[static_cast<size_t> (row)];
            int y = row * cellSize;

            if (rowInfo.isSection)
            {
                // Section header row
                drawSectionHeader (g, row, y, rowInfo.id);
            }
            else
            {
                // Item row
                drawItemRow (g, row, y, rowInfo.id);
            }
        }

        // Draw grid lines
        g.setColour (colors.chromeDivider.withAlpha (0.3f));
        for (int row = 0; row <= static_cast<int> (visibleRows.size()); ++row)
        {
            int y = row * cellSize;
            g.drawHorizontalLine (y, 0.0f, static_cast<float> (getWidth()));
        }
        for (int col = 0; col <= numChannels; ++col)
        {
            int x = paramLabelWidth + col * cellSize;
            g.drawVerticalLine (x, 0.0f, static_cast<float> (getHeight()));
        }
    }

    void drawSectionHeader (juce::Graphics& g, int /*row*/, int y, const juce::String& sectionId)
    {
        const auto& colors = ColorScheme::get();
        bool expanded = expandedSections[sectionId];

        // Section background
        g.setColour (colors.backgroundAlt);
        g.fillRect (0, y, paramLabelWidth, cellSize);

        // Triangle icon
        juce::Path triangle;
        float triX = 8.0f;
        float triY = static_cast<float> (y) + cellSize / 2.0f;
        float triSize = 8.0f;

        if (expanded)
        {
            // Pointing down
            triangle.addTriangle (triX, triY - triSize / 2,
                                  triX + triSize, triY - triSize / 2,
                                  triX + triSize / 2, triY + triSize / 2);
        }
        else
        {
            // Pointing right
            triangle.addTriangle (triX, triY - triSize / 2,
                                  triX, triY + triSize / 2,
                                  triX + triSize, triY);
        }

        g.setColour (colors.textPrimary);
        g.fillPath (triangle);

        // Section name
        juce::String displayName = getSectionDisplayName (sectionId);
        g.setFont (juce::Font (juce::FontOptions (juce::jmax(10.0f, 14.0f * WfsLookAndFeel::uiScale)).withStyle ("Bold")));
        g.drawText (displayName, 22, y, paramLabelWidth - 26, cellSize, juce::Justification::centredLeft);

        // Draw section state cells for each channel
        for (int ch = 0; ch < numChannels; ++ch)
        {
            int x = paramLabelWidth + ch * cellSize;
            auto chState = scope.getSectionStateForChannel (juce::Identifier (sectionId), ch);
            drawStateCell (g, x, y, chState, true);

            // Draw dirty earmark if any item in this section was modified
            if (dirtyTracker != nullptr)
            {
                for (const auto* item : ExtendedScope::getItemsForSection (juce::Identifier (sectionId)))
                {
                    if (dirtyTracker->isDirty (item->itemId, ch))
                    {
                        drawDirtyEarmark (g, x, y);
                        break;
                    }
                }
            }
        }
    }

    void drawItemRow (juce::Graphics& g, int /*row*/, int y, const juce::String& itemId)
    {
        const auto& colors = ColorScheme::get();

        // Item label
        g.setColour (colors.surfaceCard);
        g.fillRect (0, y, paramLabelWidth, cellSize);

        juce::String displayName = getItemDisplayName (itemId);
        g.setColour (colors.textPrimary);
        g.setFont (juce::Font (juce::FontOptions (juce::jmax(8.0f, 12.0f * WfsLookAndFeel::uiScale))));
        g.drawText (displayName, 22, y, paramLabelWidth - 26, cellSize, juce::Justification::centredLeft);

        // Draw cells for each channel
        for (int ch = 0; ch < numChannels; ++ch)
        {
            int x = paramLabelWidth + ch * cellSize;
            bool included = scope.isIncluded (itemId, ch);
            drawStateCell (g, x, y, included ? InclusionState::AllIncluded : InclusionState::AllExcluded, false);

            // Draw dirty earmark if parameter was modified by user
            if (dirtyTracker != nullptr && dirtyTracker->isDirty (itemId, ch))
                drawDirtyEarmark (g, x, y);
        }
    }

    void drawStateCell (juce::Graphics& g, int x, int y, InclusionState state, bool /*isSection*/)
    {
        const auto& colors = ColorScheme::get();
        auto cellBounds = juce::Rectangle<float> (static_cast<float> (x) + 2, static_cast<float> (y) + 2,
                                                   static_cast<float> (cellSize) - 4, static_cast<float> (cellSize) - 4);

        if (state == InclusionState::AllIncluded)
        {
            g.setColour (colors.accentGreen);
            g.fillRoundedRectangle (cellBounds, 3.0f);
        }
        else if (state == InclusionState::AllExcluded)
        {
            g.setColour (colors.surfaceCard.darker (0.15f));
            g.fillRoundedRectangle (cellBounds, 3.0f);
            g.setColour (colors.chromeDivider);
            g.drawRoundedRectangle (cellBounds, 3.0f, 1.0f);
        }
        else  // Partial
        {
            // Draw striped pattern
            drawStripedPattern (g, cellBounds);
        }
    }

    void drawStripedPattern (juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        const auto& colors = ColorScheme::get();
        auto greenColor = colors.accentGreen;
        auto greyColor = colors.surfaceCard.darker (0.15f);

        g.saveState();
        g.reduceClipRegion (bounds.toNearestInt());

        // Fill background with grey
        g.setColour (greyColor);
        g.fillRoundedRectangle (bounds, 3.0f);

        // Draw diagonal green stripes
        const float stripeWidth = 4.0f;
        float startX = bounds.getX() - bounds.getHeight();

        g.setColour (greenColor);
        for (float sx = startX; sx < bounds.getRight(); sx += stripeWidth * 2)
        {
            juce::Path stripe;
            stripe.startNewSubPath (sx, bounds.getBottom());
            stripe.lineTo (sx + bounds.getHeight(), bounds.getY());
            stripe.lineTo (sx + bounds.getHeight() + stripeWidth, bounds.getY());
            stripe.lineTo (sx + stripeWidth, bounds.getBottom());
            stripe.closeSubPath();
            g.fillPath (stripe);
        }

        g.restoreState();
    }

    void drawDirtyEarmark (juce::Graphics& g, int x, int y)
    {
        float sz = static_cast<float> (cellSize) * 0.35f;
        float cx = static_cast<float> (x) + static_cast<float> (cellSize) - 2.0f;
        float cy = static_cast<float> (y) + 2.0f;

        juce::Path earmark;
        earmark.addTriangle (cx - sz, cy, cx, cy, cx, cy + sz);

        g.setColour (juce::Colour (0xFFE6B800));  // Golden yellow
        g.fillPath (earmark);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        int row = e.y / cellSize;
        int col = (e.x - paramLabelWidth) / cellSize;

        if (row < 0 || row >= static_cast<int> (visibleRows.size()))
            return;

        const auto& rowInfo = visibleRows[static_cast<size_t> (row)];

        if (e.x < paramLabelWidth)
        {
            // Clicked on label area
            if (rowInfo.isSection)
            {
                // Check if clicked on triangle area (expand/collapse)
                if (e.x < 20)
                {
                    expandedSections[rowInfo.id] = !expandedSections[rowInfo.id];
                    buildLayout();
                    if (onLayoutChanged)
                        onLayoutChanged();
                }
                else
                {
                    // Toggle entire section for all channels
                    auto sectionId = juce::Identifier (rowInfo.id);
                    auto state = scope.getSectionState (sectionId, numChannels);
                    bool newState = (state != InclusionState::AllIncluded);
                    scope.setSectionForAllChannels (sectionId, newState, numChannels);
                }
            }
            else
            {
                // Toggle item for all channels
                auto state = InclusionState::AllIncluded;
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    if (!scope.isIncluded (rowInfo.id, ch))
                    {
                        state = InclusionState::Partial;
                        break;
                    }
                }
                bool newState = (state != InclusionState::AllIncluded);
                scope.setItemForAllChannels (rowInfo.id, newState, numChannels);
            }
        }
        else if (col >= 0 && col < numChannels)
        {
            // Clicked on a cell
            if (rowInfo.isSection)
            {
                // Toggle section for this channel
                auto sectionId = juce::Identifier (rowInfo.id);
                auto state = scope.getSectionStateForChannel (sectionId, col);
                bool newState = (state != InclusionState::AllIncluded);

                for (const auto* item : ExtendedScope::getItemsForSection (sectionId))
                    scope.setIncluded (item->itemId, col, newState);
            }
            else
            {
                // Toggle single item for this channel
                scope.toggle (rowInfo.id, col);
            }
        }

        repaint();
        if (onScopeChanged)
            onScopeChanged();
    }

    juce::String getSectionDisplayName (const juce::String& sectionId) const
    {
        if (sectionId == "Channel") return LOC("snapshotScope.sections.input");
        if (sectionId == "Position") return LOC("snapshotScope.sections.position");
        if (sectionId == "Attenuation") return LOC("snapshotScope.sections.attenuation");
        if (sectionId == "Directivity") return LOC("snapshotScope.sections.directivity");
        if (sectionId == "LiveSourceTamer") return LOC("snapshotScope.sections.liveSource");
        if (sectionId == "Hackoustics") return LOC("snapshotScope.sections.hackoustics");
        if (sectionId == "LFO") return LOC("snapshotScope.sections.lfo");
        if (sectionId == "AutomOtion") return LOC("snapshotScope.sections.automOtion");
        if (sectionId == "Mutes") return LOC("snapshotScope.sections.mutes");
        return sectionId;
    }

    juce::String getItemDisplayName (const juce::String& itemId) const
    {
        for (const auto& item : ExtendedScope::getScopeItems())
        {
            if (item.itemId == itemId)
                return item.displayName;
        }
        return itemId;
    }

    std::function<void()> onScopeChanged;
    std::function<void()> onLayoutChanged;

    int cellSize = 22;
    int paramLabelWidth = 140;

    void updateScaledSizes()
    {
        const float us = WfsLookAndFeel::uiScale;
        cellSize = juce::jmax(15, static_cast<int>(22.0f * us));
        paramLabelWidth = juce::jmax(90, static_cast<int>(140.0f * us));
    }

private:
    struct RowInfo
    {
        juce::String id;
        bool isSection;
        bool expanded;
    };

    ExtendedScope& scope;
    int numChannels;
    const ParameterDirtyTracker* dirtyTracker = nullptr;
    std::vector<RowInfo> visibleRows;
    std::map<juce::String, bool> expandedSections;
    std::map<juce::String, int> sectionStartRows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopeGridComponent)
};

//==============================================================================
// Channel Header Component
//==============================================================================
class ScopeChannelHeader : public juce::Component
{
public:
    using ExtendedScope = WFSFileManager::ExtendedSnapshotScope;
    using InclusionState = ExtendedScope::InclusionState;

    ScopeChannelHeader (ExtendedScope& scopeRef, int numChannelsValue)
        : scope (scopeRef), numChannels (numChannelsValue)
    {
    }

    void paint (juce::Graphics& g) override
    {
        const auto& colors = ColorScheme::get();
        g.fillAll (colors.backgroundAlt);

        // Draw "All" button in corner
        auto allBounds = juce::Rectangle<float> (2.0f, 2.0f,
                                                  static_cast<float> (paramLabelWidth) - 4.0f,
                                                  static_cast<float> (headerHeight) - 4.0f);

        auto overallState = scope.getOverallState (numChannels);
        if (overallState == InclusionState::AllIncluded)
            g.setColour (colors.accentGreen);
        else if (overallState == InclusionState::AllExcluded)
            g.setColour (colors.surfaceCard.darker (0.15f));
        else
            g.setColour (colors.accentGreen.interpolatedWith (colors.surfaceCard.darker (0.15f), 0.5f));

        g.fillRoundedRectangle (allBounds, 3.0f);
        g.setColour (colors.textPrimary);
        g.setFont (juce::Font (juce::FontOptions (juce::jmax(8.0f, 11.0f * WfsLookAndFeel::uiScale)).withStyle ("Bold")));
        g.drawText (LOC("snapshotScope.all"), allBounds.toNearestInt(), juce::Justification::centred);

        // Draw channel numbers
        for (int ch = 0; ch < numChannels; ++ch)
        {
            int x = paramLabelWidth + ch * cellSize;
            auto chState = scope.getChannelState (ch);

            auto cellBounds = juce::Rectangle<float> (static_cast<float> (x) + 2.0f, 2.0f,
                                                       static_cast<float> (cellSize) - 4.0f,
                                                       static_cast<float> (headerHeight) - 4.0f);

            if (chState == InclusionState::AllIncluded)
                g.setColour (colors.accentGreen);
            else if (chState == InclusionState::AllExcluded)
                g.setColour (colors.surfaceCard.darker (0.15f));
            else
                g.setColour (colors.accentGreen.interpolatedWith (colors.surfaceCard.darker (0.15f), 0.5f));

            g.fillRoundedRectangle (cellBounds, 3.0f);

            g.setColour (colors.textPrimary);
            g.setFont (juce::Font (juce::FontOptions (juce::jmax(7.0f, 10.0f * WfsLookAndFeel::uiScale))));
            g.drawText (juce::String (ch + 1), cellBounds.toNearestInt(), juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.x < paramLabelWidth)
        {
            // Clicked on "All" button - toggle everything
            auto state = scope.getOverallState (numChannels);
            bool newState = (state != InclusionState::AllIncluded);
            scope.setAll (newState, numChannels);
        }
        else
        {
            // Clicked on channel header - toggle entire channel
            int ch = (e.x - paramLabelWidth) / cellSize;
            if (ch >= 0 && ch < numChannels)
            {
                auto state = scope.getChannelState (ch);
                bool newState = (state != InclusionState::AllIncluded);
                scope.setAllItemsForChannel (ch, newState);
            }
        }

        repaint();
        if (onScopeChanged)
            onScopeChanged();
    }

    std::function<void()> onScopeChanged;

    int cellSize = 22;
    int paramLabelWidth = 140;
    int headerHeight = 24;

    void updateScaledSizes()
    {
        const float us = WfsLookAndFeel::uiScale;
        cellSize = juce::jmax(15, static_cast<int>(22.0f * us));
        paramLabelWidth = juce::jmax(90, static_cast<int>(140.0f * us));
        headerHeight = juce::jmax(16, static_cast<int>(24.0f * us));
    }

private:
    ExtendedScope& scope;
    int numChannels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopeChannelHeader)
};

//==============================================================================
// Snapshot Scope Content Component
//==============================================================================
class SnapshotScopeContent : public juce::Component,
                              public ColorScheme::Manager::Listener
{
public:
    using ExtendedScope = WFSFileManager::ExtendedSnapshotScope;

    SnapshotScopeContent (WfsParameters& params, const juce::String& snapshotNameValue, ExtendedScope& scopeRef,
                          ParameterDirtyTracker* dirtyTrackerPtr = nullptr)
        : parameters (params),
          snapshotName (snapshotNameValue),
          scope (scopeRef),
          numChannels (params.getNumInputChannels()),
          dirtyTracker (dirtyTrackerPtr)
    {
        ColorScheme::Manager::getInstance().addListener (this);

        // Title label
        addAndMakeVisible (titleLabel);
        titleLabel.setText (LOC("snapshotScope.title").replace("{name}", snapshotName), juce::dontSendNotification);
        titleLabel.setFont (juce::Font (juce::FontOptions (16.0f).withStyle ("Bold")));
        titleLabel.setJustificationType (juce::Justification::centred);

        // Apply mode radio buttons
        addAndMakeVisible (applyModeLabel);
        applyModeLabel.setText (LOC("snapshotScope.applyScope"), juce::dontSendNotification);

        addAndMakeVisible (applySavingButton);
        applySavingButton.setButtonText (LOC("snapshotScope.whenSaving"));
        applySavingButton.setRadioGroupId (1);
        applySavingButton.setClickingTogglesState (true);
        applySavingButton.onClick = [this]() {
            scope.applyMode = ExtendedScope::ApplyMode::OnSave;
            updateSnapshotLoadCueVisibility();
        };

        addAndMakeVisible (applyRecallingButton);
        applyRecallingButton.setButtonText (LOC("snapshotScope.whenRecalling"));
        applyRecallingButton.setRadioGroupId (1);
        applyRecallingButton.setClickingTogglesState (true);
        applyRecallingButton.onClick = [this]() {
            scope.applyMode = ExtendedScope::ApplyMode::OnRecall;
            updateSnapshotLoadCueVisibility();
        };

        // Set initial state
        if (scope.applyMode == ExtendedScope::ApplyMode::OnSave)
            applySavingButton.setToggleState (true, juce::dontSendNotification);
        else
            applyRecallingButton.setToggleState (true, juce::dontSendNotification);

        // Channel header (fixed at top)
        channelHeader = std::make_unique<ScopeChannelHeader> (scope, numChannels);
        addAndMakeVisible (channelHeader.get());
        channelHeader->onScopeChanged = [this]() { gridComponent->repaint(); };

        // Scrollable grid (pass dirty tracker for earmarks)
        gridComponent = std::make_unique<ScopeGridComponent> (scope, numChannels, dirtyTracker);
        gridComponent->onScopeChanged = [this]() { channelHeader->repaint(); };
        gridComponent->onLayoutChanged = [this]() { resized(); };

        addAndMakeVisible (viewport);
        viewport.setViewedComponent (gridComponent.get(), false);
        viewport.setScrollBarsShown (true, true);

        // Write to QLab radio option (exclusive with save/recall)
        addAndMakeVisible (writeToQLabToggle);
        writeToQLabToggle.setButtonText (LOC("snapshotScope.writeToQLab"));
        writeToQLabToggle.setTooltip (LOC("snapshotScope.writeToQLabTooltip"));
        writeToQLabToggle.setRadioGroupId (1);
        writeToQLabToggle.setClickingTogglesState (true);
        writeToQLabToggle.onClick = [this]() {
            updateSnapshotLoadCueVisibility();
        };

        // "Write Snapshot Load Cue to QLab" checkbox (additive, not radio)
        addAndMakeVisible (writeSnapshotLoadCueToggle);
        writeSnapshotLoadCueToggle.setButtonText (LOC("snapshotScope.writeSnapshotLoadCue"));
        writeSnapshotLoadCueToggle.setTooltip (LOC("snapshotScope.writeSnapshotLoadCueTooltip"));
        writeSnapshotLoadCueToggle.setToggleState (false, juce::dontSendNotification);

        // Dirty tracking controls
        addAndMakeVisible (autoPreselectToggle);
        autoPreselectToggle.setButtonText (LOC("snapshotScope.autoPreselectModified"));
        {
            // Load persisted toggle state from config
            auto config = params.getValueTreeState().getConfigState();
            auto showSection = config.getChildWithName (WFSParameterIDs::Show);
            bool savedState = showSection.isValid()
                                  ? static_cast<bool> (showSection.getProperty (WFSParameterIDs::autoPreselectDirty, false))
                                  : false;
            autoPreselectToggle.setToggleState (savedState, juce::dontSendNotification);
        }
        autoPreselectToggle.onClick = [this]() {
            // Persist the toggle state
            auto config = parameters.getValueTreeState().getConfigState();
            auto showSection = config.getChildWithName (WFSParameterIDs::Show);
            if (showSection.isValid())
                showSection.setProperty (WFSParameterIDs::autoPreselectDirty,
                                         autoPreselectToggle.getToggleState(), nullptr);

            // Apply immediately when toggled ON
            if (autoPreselectToggle.getToggleState())
                applyDirtyToScope();

            updateSelectModifiedVisibility();
        };

        addAndMakeVisible (selectModifiedButton);
        selectModifiedButton.setButtonText (LOC("snapshotScope.buttons.selectModified"));
        selectModifiedButton.setEnabled (dirtyTracker != nullptr && dirtyTracker->hasAnyDirty());
        selectModifiedButton.onClick = [this]() {
            applyDirtyToScope();
        };

        addChildComponent (clearChangesButton);
        clearChangesButton.setButtonText (LOC("snapshotScope.buttons.clearChanges"));
        clearChangesButton.setVisible (dirtyTracker != nullptr && dirtyTracker->hasAnyDirty());
        clearChangesButton.onClick = [this]() {
            if (dirtyTracker != nullptr)
                dirtyTracker->clearAll();
        };

        // Live dirty-state updates: repaint grid + update button when dirty flags change
        if (dirtyTracker != nullptr)
        {
            dirtyTracker->onDirtyStateChanged = [this]() {
                bool hasDirty = dirtyTracker != nullptr && dirtyTracker->hasAnyDirty();
                selectModifiedButton.setEnabled (hasDirty);
                clearChangesButton.setVisible (hasDirty);
                gridComponent->repaint();
                channelHeader->repaint();

                // Continuous auto-apply: update scope selection live when toggle is ON
                if (autoPreselectToggle.getToggleState())
                    applyDirtyToScope();
            };
        }

        // Hide "Select Modified" button when auto-preselect is ON (redundant)
        selectModifiedButton.setVisible (!autoPreselectToggle.getToggleState());

        // Auto-preselect on open if toggle is ON and there are dirty params
        if (autoPreselectToggle.getToggleState() && dirtyTracker != nullptr && dirtyTracker->hasAnyDirty())
            applyDirtyToScope();

        // Action buttons
        addAndMakeVisible (saveButton);
        saveButton.setButtonText (LOC("snapshotScope.buttons.ok"));
        saveButton.onClick = [this]() {
            if (onSaveRequested)
                onSaveRequested (writeToQLabToggle.getToggleState(),
                                 writeSnapshotLoadCueToggle.getToggleState());
        };

        addAndMakeVisible (cancelButton);
        cancelButton.setButtonText (LOC("snapshotScope.buttons.cancel"));
        cancelButton.onClick = [this]() {
            if (onCloseRequested)
                onCloseRequested();
        };

        applyTheme();
    }

    ~SnapshotScopeContent() override
    {
        if (dirtyTracker != nullptr)
            dirtyTracker->onDirtyStateChanged = nullptr;
        ColorScheme::Manager::getInstance().removeListener (this);
    }

    void colorSchemeChanged() override
    {
        applyTheme();
        repaint();
    }

    void applyTheme()
    {
        const auto& colors = ColorScheme::get();

        titleLabel.setColour (juce::Label::textColourId, colors.textPrimary);
        applyModeLabel.setColour (juce::Label::textColourId, colors.textPrimary);

        saveButton.setColour (juce::TextButton::buttonColourId, colors.accentGreen);
        saveButton.setColour (juce::TextButton::textColourOffId, colors.textPrimary);

        cancelButton.setColour (juce::TextButton::buttonColourId, colors.buttonNormal);
        cancelButton.setColour (juce::TextButton::textColourOffId, colors.textPrimary);

        selectModifiedButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFFB8960F));
        selectModifiedButton.setColour (juce::TextButton::textColourOffId, colors.textPrimary);

        clearChangesButton.setColour (juce::TextButton::buttonColourId, colors.buttonNormal);
        clearChangesButton.setColour (juce::TextButton::textColourOffId, colors.textPrimary);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (ColorScheme::get().background);
    }

    void resized() override
    {
        float ls = static_cast<float>(getHeight()) / 600.0f;
        auto sc = [ls](int ref) { return juce::jmax(static_cast<int>(ref * 0.65f), static_cast<int>(ref * ls)); };

        auto bounds = getLocalBounds().reduced (sc(10));

        // Title
        titleLabel.setBounds (bounds.removeFromTop (sc(30)));
        bounds.removeFromTop (sc(5));

        // Apply mode row (save / recall / QLab — mutually exclusive)
        auto modeRow = bounds.removeFromTop (sc(28));
        applyModeLabel.setBounds (modeRow.removeFromLeft (sc(90)));
        applySavingButton.setBounds (modeRow.removeFromLeft (sc(120)));
        modeRow.removeFromLeft (sc(10));
        applyRecallingButton.setBounds (modeRow.removeFromLeft (sc(140)));
        modeRow.removeFromLeft (sc(10));
        writeToQLabToggle.setBounds (modeRow.removeFromLeft (sc(140)));
        bounds.removeFromTop (sc(5));

        // Snapshot load cue checkbox (below mode row, indented)
        if (writeSnapshotLoadCueToggle.isVisible())
        {
            auto loadCueRow = bounds.removeFromTop (sc(24));
            loadCueRow.removeFromLeft (sc(90));  // Align with radio buttons
            writeSnapshotLoadCueToggle.setBounds (loadCueRow.removeFromLeft (sc(300)));
        }
        bounds.removeFromTop (sc(5));

        // Dirty tracking row (auto-preselect toggle + select modified + clear changes)
        auto dirtyRow = bounds.removeFromTop (sc(28));
        autoPreselectToggle.setBounds (dirtyRow.removeFromLeft (sc(280)));
        dirtyRow.removeFromLeft (sc(10));
        if (selectModifiedButton.isVisible())
            selectModifiedButton.setBounds (dirtyRow.removeFromLeft (sc(130)));
        else
            dirtyRow.removeFromLeft (sc(130));  // reserve space to keep Clear Changes fixed
        dirtyRow.removeFromLeft (sc(10));
        clearChangesButton.setBounds (dirtyRow.removeFromLeft (sc(130)));
        bounds.removeFromTop (sc(5));

        // Action buttons at bottom
        auto buttonRow = bounds.removeFromBottom (sc(35));
        bounds.removeFromBottom (sc(5));

        int buttonWidth = sc(100);
        int buttonSpacing = sc(20);
        int totalButtonWidth = buttonWidth * 2 + buttonSpacing;
        int buttonX = (buttonRow.getWidth() - totalButtonWidth) / 2;

        saveButton.setBounds (buttonX, buttonRow.getY(), buttonWidth, sc(30));
        cancelButton.setBounds (buttonX + buttonWidth + buttonSpacing, buttonRow.getY(), buttonWidth, sc(30));

        // Update scaled sizes for grid and header
        gridComponent->updateScaledSizes();
        channelHeader->updateScaledSizes();

        // Channel header (fixed)
        int gridWidth = gridComponent->paramLabelWidth + numChannels * gridComponent->cellSize;
        channelHeader->setBounds (bounds.getX(), bounds.getY(), gridWidth, channelHeader->headerHeight);
        bounds.removeFromTop (channelHeader->headerHeight);

        // Viewport for grid
        viewport.setBounds (bounds);
    }

    std::function<void()> onCloseRequested;
    std::function<void(bool writeToQLab, bool writeSnapshotLoadCue)> onSaveRequested;

    /** Enable/disable the QLab radio option based on whether a QLab target exists */
    void setQLabAvailable (bool available)
    {
        qlabAvailable = available;
        writeToQLabToggle.setEnabled (available);
        writeToQLabToggle.setAlpha (available ? 1.0f : 0.4f);

        // If QLab was selected but becomes unavailable, fall back to recall mode
        if (!available && writeToQLabToggle.getToggleState())
        {
            applyRecallingButton.setToggleState (true, juce::sendNotification);
        }

        updateSnapshotLoadCueVisibility();
    }

private:
    WfsParameters& parameters;
    juce::String snapshotName;
    ExtendedScope& scope;
    int numChannels;

    juce::Label titleLabel;
    juce::Label applyModeLabel;
    juce::ToggleButton applySavingButton;
    juce::ToggleButton applyRecallingButton;

    std::unique_ptr<ScopeChannelHeader> channelHeader;
    std::unique_ptr<ScopeGridComponent> gridComponent;
    juce::Viewport viewport;

    juce::ToggleButton writeToQLabToggle;
    juce::ToggleButton writeSnapshotLoadCueToggle;
    juce::ToggleButton autoPreselectToggle;
    juce::TextButton selectModifiedButton;
    juce::TextButton clearChangesButton;
    juce::TextButton saveButton;
    juce::TextButton cancelButton;
    bool qlabAvailable = false;
    ParameterDirtyTracker* dirtyTracker = nullptr;

    /** Copy dirty flags to scope selection: dirty items included, others excluded */
    void applyDirtyToScope()
    {
        if (dirtyTracker == nullptr || !dirtyTracker->hasAnyDirty())
            return;

        const auto& dirtyKeys = dirtyTracker->getDirtyKeys();

        for (const auto& item : ExtendedScope::getScopeItems())
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto key = ExtendedScope::makeKey (item.itemId, ch);
                scope.itemChannelStates[key] = (dirtyKeys.find (key) != dirtyKeys.end());
            }
        }

        gridComponent->repaint();
        channelHeader->repaint();
    }

    void updateSelectModifiedVisibility()
    {
        selectModifiedButton.setVisible (!autoPreselectToggle.getToggleState());
        resized();
    }

    void updateSnapshotLoadCueVisibility()
    {
        bool saveOrRecallMode = !writeToQLabToggle.getToggleState();
        bool shouldShow = qlabAvailable && saveOrRecallMode;

        writeSnapshotLoadCueToggle.setVisible (shouldShow);
        writeSnapshotLoadCueToggle.setEnabled (shouldShow);
        resized();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnapshotScopeContent)
};

//==============================================================================
// Snapshot Scope Window
//==============================================================================
class SnapshotScopeWindow : public juce::DocumentWindow,
                            public ColorScheme::Manager::Listener
{
public:
    using ExtendedScope = WFSFileManager::ExtendedSnapshotScope;

    SnapshotScopeWindow (WfsParameters& params, const juce::String& snapshotName, ExtendedScope& scope,
                         ParameterDirtyTracker* dirtyTracker = nullptr)
        : DocumentWindow (LOC("snapshotScope.windowTitle"),
                          ColorScheme::get().background,
                          DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar (true);
        setResizable (true, true);

        content = std::make_unique<SnapshotScopeContent> (params, snapshotName, scope, dirtyTracker);
        content->onCloseRequested = [this]() { closeButtonPressed(); };
        content->onSaveRequested = [this](bool writeQLab, bool writeLoadCue) {
            saved = true;
            writeToQLab = writeQLab;
            writeSnapshotLoadCue = writeLoadCue;
            closeButtonPressed();
        };

        setContentOwned (content.release(), false);

        // Size based on number of channels, scaled with display resolution
        auto* disp = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
        float ds = (disp && !disp->userArea.isEmpty()) ? static_cast<float>(disp->userArea.getHeight()) / 1080.0f : 1.0f;
        auto dsc = [ds](int ref) { return juce::jmax(static_cast<int>(ref * 0.65f), static_cast<int>(ref * ds)); };

        int numChannels = params.getNumInputChannels();
        int scaledCellSize = juce::jmax(15, static_cast<int>(22.0f * ds));
        int scaledParamLabelWidth = juce::jmax(90, static_cast<int>(140.0f * ds));
        int gridWidth = scaledParamLabelWidth + numChannels * scaledCellSize + dsc(50);
        int width = juce::jmax (dsc(600), juce::jmin (dsc(1200), gridWidth));
        int height = dsc(600);
        centreWithSize (width, height);
        setVisible (true);
        WindowUtils::enableDarkTitleBar (this);

        ColorScheme::Manager::getInstance().addListener (this);
    }

    ~SnapshotScopeWindow() override
    {
        ColorScheme::Manager::getInstance().removeListener (this);
    }

    void closeButtonPressed() override
    {
        setVisible (false);
        if (onWindowClosed)
            onWindowClosed (saved, writeToQLab, writeSnapshotLoadCue);
    }

    void colorSchemeChanged() override
    {
        setBackgroundColour (ColorScheme::get().background);
        repaint();
    }

    std::function<void (bool saved, bool writeToQLab, bool writeSnapshotLoadCue)> onWindowClosed;

    /** Set whether QLab export is available (pass through to content) */
    void setQLabAvailable (bool available)
    {
        if (auto* c = dynamic_cast<SnapshotScopeContent*> (getContentComponent()))
            c->setQLabAvailable (available);
    }

private:
    std::unique_ptr<SnapshotScopeContent> content;
    bool saved = false;
    bool writeToQLab = false;
    bool writeSnapshotLoadCue = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnapshotScopeWindow)
};
