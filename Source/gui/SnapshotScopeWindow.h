#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSFileManager.h"
#include "../Localization/LocalizationManager.h"
#include "ColorScheme.h"
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

    ScopeGridComponent (ExtendedScope& scopeRef, int numChannelsValue)
        : scope (scopeRef), numChannels (numChannelsValue)
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
        g.setFont (juce::Font (juce::FontOptions (14.0f).withStyle ("Bold")));
        g.drawText (displayName, 22, y, paramLabelWidth - 26, cellSize, juce::Justification::centredLeft);

        // Draw section state cells for each channel
        for (int ch = 0; ch < numChannels; ++ch)
        {
            int x = paramLabelWidth + ch * cellSize;
            auto chState = scope.getSectionStateForChannel (juce::Identifier (sectionId), ch);
            drawStateCell (g, x, y, chState, true);
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
        g.setFont (juce::Font (juce::FontOptions (12.0f)));
        g.drawText (displayName, 22, y, paramLabelWidth - 26, cellSize, juce::Justification::centredLeft);

        // Draw cells for each channel
        for (int ch = 0; ch < numChannels; ++ch)
        {
            int x = paramLabelWidth + ch * cellSize;
            bool included = scope.isIncluded (itemId, ch);
            drawStateCell (g, x, y, included ? InclusionState::AllIncluded : InclusionState::AllExcluded, false);
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

    static constexpr int cellSize = 22;
    static constexpr int paramLabelWidth = 140;

private:
    struct RowInfo
    {
        juce::String id;
        bool isSection;
        bool expanded;
    };

    ExtendedScope& scope;
    int numChannels;
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
        g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
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
            g.setFont (juce::Font (juce::FontOptions (10.0f)));
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

    static constexpr int cellSize = ScopeGridComponent::cellSize;
    static constexpr int paramLabelWidth = ScopeGridComponent::paramLabelWidth;
    static constexpr int headerHeight = 24;

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

    SnapshotScopeContent (WfsParameters& params, const juce::String& snapshotNameValue, ExtendedScope& scopeRef)
        : parameters (params),
          snapshotName (snapshotNameValue),
          scope (scopeRef),
          numChannels (params.getNumInputChannels())
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
        };

        addAndMakeVisible (applyRecallingButton);
        applyRecallingButton.setButtonText (LOC("snapshotScope.whenRecalling"));
        applyRecallingButton.setRadioGroupId (1);
        applyRecallingButton.setClickingTogglesState (true);
        applyRecallingButton.onClick = [this]() {
            scope.applyMode = ExtendedScope::ApplyMode::OnRecall;
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

        // Scrollable grid
        gridComponent = std::make_unique<ScopeGridComponent> (scope, numChannels);
        gridComponent->onScopeChanged = [this]() { channelHeader->repaint(); };
        gridComponent->onLayoutChanged = [this]() { resized(); };

        addAndMakeVisible (viewport);
        viewport.setViewedComponent (gridComponent.get(), false);
        viewport.setScrollBarsShown (true, true);

        // Action buttons
        addAndMakeVisible (saveButton);
        saveButton.setButtonText (LOC("snapshotScope.buttons.save"));
        saveButton.onClick = [this]() {
            if (onSaveRequested)
                onSaveRequested();
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
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (ColorScheme::get().background);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (10);

        // Title
        titleLabel.setBounds (bounds.removeFromTop (30));
        bounds.removeFromTop (5);

        // Apply mode row
        auto modeRow = bounds.removeFromTop (28);
        applyModeLabel.setBounds (modeRow.removeFromLeft (90));
        applySavingButton.setBounds (modeRow.removeFromLeft (120));
        modeRow.removeFromLeft (10);
        applyRecallingButton.setBounds (modeRow.removeFromLeft (140));
        bounds.removeFromTop (10);

        // Action buttons at bottom
        auto buttonRow = bounds.removeFromBottom (35);
        bounds.removeFromBottom (5);

        int buttonWidth = 100;
        int buttonSpacing = 20;
        int totalButtonWidth = buttonWidth * 2 + buttonSpacing;
        int buttonX = (buttonRow.getWidth() - totalButtonWidth) / 2;

        saveButton.setBounds (buttonX, buttonRow.getY(), buttonWidth, 30);
        cancelButton.setBounds (buttonX + buttonWidth + buttonSpacing, buttonRow.getY(), buttonWidth, 30);

        // Channel header (fixed)
        int gridWidth = ScopeGridComponent::paramLabelWidth + numChannels * ScopeGridComponent::cellSize;
        channelHeader->setBounds (bounds.getX(), bounds.getY(), gridWidth, ScopeChannelHeader::headerHeight);
        bounds.removeFromTop (ScopeChannelHeader::headerHeight);

        // Viewport for grid
        viewport.setBounds (bounds);
    }

    std::function<void()> onCloseRequested;
    std::function<void()> onSaveRequested;

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

    juce::TextButton saveButton;
    juce::TextButton cancelButton;

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

    SnapshotScopeWindow (WfsParameters& params, const juce::String& snapshotName, ExtendedScope& scope)
        : DocumentWindow (LOC("snapshotScope.windowTitle"),
                          ColorScheme::get().background,
                          DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar (true);
        setResizable (true, true);

        content = std::make_unique<SnapshotScopeContent> (params, snapshotName, scope);
        content->onCloseRequested = [this]() { closeButtonPressed(); };
        content->onSaveRequested = [this]() {
            saved = true;
            closeButtonPressed();
        };

        setContentOwned (content.release(), false);

        // Size based on number of channels
        int numChannels = params.getNumInputChannels();
        int width = juce::jmin (1200, ScopeGridComponent::paramLabelWidth + numChannels * ScopeGridComponent::cellSize + 50);
        int height = 600;
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
            onWindowClosed (saved);
    }

    void colorSchemeChanged() override
    {
        setBackgroundColour (ColorScheme::get().background);
        repaint();
    }

    std::function<void (bool saved)> onWindowClosed;

private:
    std::unique_ptr<SnapshotScopeContent> content;
    bool saved = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnapshotScopeWindow)
};
