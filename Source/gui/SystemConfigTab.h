#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "StatusBar.h"
#include "ColorScheme.h"

#if JUCE_WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "ws2_32.lib")
#endif

//==============================================================================
// Custom Origin Preset Button - Front (broken rectangle with dot at bottom)
class OriginFrontButton : public juce::Button
{
public:
    OriginFrontButton() : juce::Button("Front") {}

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // Background - use theme colors
        if (shouldDrawButtonAsDown)
            g.setColour(ColorScheme::get().buttonPressed);
        else if (shouldDrawButtonAsHighlighted)
            g.setColour(ColorScheme::get().buttonHover);
        else
            g.setColour(ColorScheme::get().buttonNormal);

        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(ColorScheme::get().buttonBorder);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Draw icon - broken rectangle (open at bottom) with dot at front center
        auto iconBounds = bounds.reduced(6.0f);
        float lineThickness = 2.0f;
        float gapSize = iconBounds.getWidth() * 0.55f;  // Wider gap in the middle of bottom edge

        g.setColour(ColorScheme::get().textPrimary);

        // Left side (full height)
        g.drawLine(iconBounds.getX(), iconBounds.getY(),
                   iconBounds.getX(), iconBounds.getBottom(), lineThickness);

        // Top (full width)
        g.drawLine(iconBounds.getX(), iconBounds.getY(),
                   iconBounds.getRight(), iconBounds.getY(), lineThickness);

        // Right side (full height)
        g.drawLine(iconBounds.getRight(), iconBounds.getY(),
                   iconBounds.getRight(), iconBounds.getBottom(), lineThickness);

        // Bottom left piece (partial - leaving gap in center)
        g.drawLine(iconBounds.getX(), iconBounds.getBottom(),
                   iconBounds.getCentreX() - gapSize * 0.5f, iconBounds.getBottom(), lineThickness);

        // Bottom right piece (partial - leaving gap in center)
        g.drawLine(iconBounds.getCentreX() + gapSize * 0.5f, iconBounds.getBottom(),
                   iconBounds.getRight(), iconBounds.getBottom(), lineThickness);

        // Dot at front center (positioned at the bottom edge, in the gap)
        float dotRadius = 2.5f;
        g.fillEllipse(iconBounds.getCentreX() - dotRadius,
                      iconBounds.getBottom() - dotRadius,
                      dotRadius * 2, dotRadius * 2);
    }
};

//==============================================================================
// Custom Origin Preset Button - Center Ground (complete rectangle with dot in center)
class OriginCenterGroundButton : public juce::Button
{
public:
    OriginCenterGroundButton() : juce::Button("Center Ground") {}

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // Background - use theme colors
        if (shouldDrawButtonAsDown)
            g.setColour(ColorScheme::get().buttonPressed);
        else if (shouldDrawButtonAsHighlighted)
            g.setColour(ColorScheme::get().buttonHover);
        else
            g.setColour(ColorScheme::get().buttonNormal);

        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(ColorScheme::get().buttonBorder);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Draw icon - complete rectangle with dot in center
        auto iconBounds = bounds.reduced(6.0f);
        float lineThickness = 2.0f;

        g.setColour(ColorScheme::get().textPrimary);

        // Complete rectangle
        g.drawRect(iconBounds, lineThickness);

        // Dot in center
        float dotRadius = 2.5f;
        g.fillEllipse(iconBounds.getCentreX() - dotRadius,
                      iconBounds.getCentreY() - dotRadius,
                      dotRadius * 2, dotRadius * 2);
    }
};

//==============================================================================
// Custom Origin Preset Button - Center (3D cube with dot in center)
class OriginCenterButton : public juce::Button
{
public:
    OriginCenterButton() : juce::Button("Center") {}

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // Background - use theme colors
        if (shouldDrawButtonAsDown)
            g.setColour(ColorScheme::get().buttonPressed);
        else if (shouldDrawButtonAsHighlighted)
            g.setColour(ColorScheme::get().buttonHover);
        else
            g.setColour(ColorScheme::get().buttonNormal);

        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(ColorScheme::get().buttonBorder);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Draw icon - 3D cube with dot in center
        auto iconBounds = bounds.reduced(6.0f);
        float lineThickness = 2.0f;
        float depth = iconBounds.getWidth() * 0.3f;  // Depth offset for 3D effect

        g.setColour(ColorScheme::get().textPrimary);

        // Front face (rectangle) - positioned at bottom-left
        auto frontFace = juce::Rectangle<float>(
            iconBounds.getX(),
            iconBounds.getY() + depth,
            iconBounds.getWidth() - depth,
            iconBounds.getHeight() - depth
        );
        g.drawRect(frontFace, lineThickness);

        // Back top-right corner position
        float backRight = iconBounds.getRight();
        float backTop = iconBounds.getY();

        // Top edge of back face
        g.drawLine(frontFace.getX() + depth, backTop, backRight, backTop, lineThickness);
        // Right edge of back face
        g.drawLine(backRight, backTop, backRight, frontFace.getBottom() - depth, lineThickness);

        // Connecting lines (front to back)
        g.drawLine(frontFace.getX(), frontFace.getY(), frontFace.getX() + depth, backTop, lineThickness);
        g.drawLine(frontFace.getRight(), frontFace.getY(), backRight, backTop, lineThickness);
        g.drawLine(frontFace.getRight(), frontFace.getBottom(), backRight, frontFace.getBottom() - depth, lineThickness);

        // Dot in center of cube (visual center)
        float dotRadius = 2.5f;
        float dotX = frontFace.getCentreX() + depth * 0.5f;
        float dotY = frontFace.getCentreY() - depth * 0.5f;
        g.fillEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2, dotRadius * 2);
    }
};

/**
 * System Configuration Tab Component
 * Contains all system-level configuration:
 * - Show (name, location)
 * - I/O (input/output/reverb channels, audio interface, processing toggle)
 * - Stage (dimensions, origin, speed of sound, temperature)
 * - Master Section (level, latency, Haas effect)
 * - Network (IP, ports, targets/servers)
 * - ADM-OSC (offset, scale, flip)
 * - Tracking (protocol, port, offset, scale, flip)
 * - Store/Reload (save/load buttons)
 */
class SystemConfigTab : public juce::Component,
                        private juce::ValueTree::Listener,
                        private juce::TextEditor::Listener,
                        public ColorScheme::Manager::Listener
{
public:
    // Callback types for notifying MainComponent of changes
    using ProcessingCallback = std::function<void(bool enabled)>;
    using ChannelCountCallback = std::function<void(int inputs, int outputs, int reverbs)>;
    using AudioInterfaceCallback = std::function<void()>;
    using ConfigReloadedCallback = std::function<void()>;

    SystemConfigTab(WfsParameters& params)
        : parameters(params)
    {
        // This tab wants keyboard focus to prevent auto-focus on first TextEditor
        setWantsKeyboardFocus(true);

        // Show Section
        addAndMakeVisible(showNameLabel);
        showNameLabel.setText("Name:", juce::dontSendNotification);
        addAndMakeVisible(showNameEditor);

        addAndMakeVisible(showLocationLabel);
        showLocationLabel.setText("Location:", juce::dontSendNotification);
        addAndMakeVisible(showLocationEditor);

        // I/O Section
        addAndMakeVisible(inputChannelsLabel);
        inputChannelsLabel.setText("Input Channels:", juce::dontSendNotification);
        addAndMakeVisible(inputChannelsEditor);

        addAndMakeVisible(outputChannelsLabel);
        outputChannelsLabel.setText("Output Channels:", juce::dontSendNotification);
        addAndMakeVisible(outputChannelsEditor);

        addAndMakeVisible(reverbChannelsLabel);
        reverbChannelsLabel.setText("Reverb Channels:", juce::dontSendNotification);
        addAndMakeVisible(reverbChannelsEditor);

        addAndMakeVisible(audioPatchingButton);
        audioPatchingButton.setButtonText("Audio Interface and Patching Window");
        audioPatchingButton.onClick = [this]() {
            if (onAudioInterfaceWindowRequested)
                onAudioInterfaceWindowRequested();
        };

        // Algorithm selector
        addAndMakeVisible(algorithmLabel);
        algorithmLabel.setText("Algorithm:", juce::dontSendNotification);

        addAndMakeVisible(algorithmSelector);
        algorithmSelector.addItem("InputBuffer (read-time delays)", 1);
        algorithmSelector.addItem("OutputBuffer (write-time delays)", 2);
        // algorithmSelector.addItem("GPU InputBuffer (GPU Audio)", 3);  // Commented out - GPU Audio SDK not configured
        algorithmSelector.setSelectedId(1, juce::dontSendNotification);
        algorithmSelector.onChange = [this]() {
            int selectedId = algorithmSelector.getSelectedId();
            parameters.setConfigParam("ProcessingAlgorithm", selectedId);
        };

        addAndMakeVisible(processingButton);
        processingButton.setButtonText("Processing: OFF");
        processingButton.onClick = [this]() { toggleProcessing(); };

        // Stage Section
        addAndMakeVisible(stageShapeLabel);
        stageShapeLabel.setText("Stage Shape:", juce::dontSendNotification);
        addAndMakeVisible(stageShapeSelector);
        stageShapeSelector.addItem("Box", 1);
        stageShapeSelector.addItem("Cylinder", 2);
        stageShapeSelector.addItem("Dome", 3);
        stageShapeSelector.setSelectedId(1, juce::dontSendNotification);
        stageShapeSelector.onChange = [this]() { onStageShapeChanged(); };

        addAndMakeVisible(stageWidthLabel);
        stageWidthLabel.setText("Width:", juce::dontSendNotification);
        addAndMakeVisible(stageWidthEditor);
        addAndMakeVisible(stageWidthUnitLabel);
        stageWidthUnitLabel.setText("m", juce::dontSendNotification);

        addAndMakeVisible(stageDepthLabel);
        stageDepthLabel.setText("Depth:", juce::dontSendNotification);
        addAndMakeVisible(stageDepthEditor);
        addAndMakeVisible(stageDepthUnitLabel);
        stageDepthUnitLabel.setText("m", juce::dontSendNotification);

        addAndMakeVisible(stageHeightLabel);
        stageHeightLabel.setText("Height:", juce::dontSendNotification);
        addAndMakeVisible(stageHeightEditor);
        addAndMakeVisible(stageHeightUnitLabel);
        stageHeightUnitLabel.setText("m", juce::dontSendNotification);

        addAndMakeVisible(stageDiameterLabel);
        stageDiameterLabel.setText("Diameter:", juce::dontSendNotification);
        addAndMakeVisible(stageDiameterEditor);
        addAndMakeVisible(stageDiameterUnitLabel);
        stageDiameterUnitLabel.setText("m", juce::dontSendNotification);

        addAndMakeVisible(domeElevationLabel);
        domeElevationLabel.setText("Elevation:", juce::dontSendNotification);
        addAndMakeVisible(domeElevationEditor);
        addAndMakeVisible(domeElevationUnitLabel);
        domeElevationUnitLabel.setText(juce::CharPointer_UTF8("\xc2\xb0"), juce::dontSendNotification);

        addAndMakeVisible(stageOriginWidthLabel);
        stageOriginWidthLabel.setText("Origin Width:", juce::dontSendNotification);
        addAndMakeVisible(stageOriginWidthEditor);
        addAndMakeVisible(stageOriginWidthUnitLabel);
        stageOriginWidthUnitLabel.setText("m", juce::dontSendNotification);

        addAndMakeVisible(stageOriginDepthLabel);
        stageOriginDepthLabel.setText("Origin Depth:", juce::dontSendNotification);
        addAndMakeVisible(stageOriginDepthEditor);
        addAndMakeVisible(stageOriginDepthUnitLabel);
        stageOriginDepthUnitLabel.setText("m", juce::dontSendNotification);

        addAndMakeVisible(stageOriginHeightLabel);
        stageOriginHeightLabel.setText("Origin Height:", juce::dontSendNotification);
        addAndMakeVisible(stageOriginHeightEditor);
        addAndMakeVisible(stageOriginHeightUnitLabel);
        stageOriginHeightUnitLabel.setText("m", juce::dontSendNotification);

        // Origin preset buttons (custom drawn icons)
        addAndMakeVisible(originFrontButton);
        originFrontButton.onClick = [this]() { setOriginToFront(); };

        addAndMakeVisible(originCenterGroundButton);
        originCenterGroundButton.onClick = [this]() { setOriginToCenterGround(); };

        addAndMakeVisible(originCenterButton);
        originCenterButton.onClick = [this]() { setOriginToCenter(); };

        addAndMakeVisible(speedOfSoundLabel);
        speedOfSoundLabel.setText("Speed of Sound:", juce::dontSendNotification);
        addAndMakeVisible(speedOfSoundEditor);
        addAndMakeVisible(speedOfSoundUnitLabel);
        speedOfSoundUnitLabel.setText("m/s", juce::dontSendNotification);

        addAndMakeVisible(temperatureLabel);
        temperatureLabel.setText("Temperature:", juce::dontSendNotification);
        addAndMakeVisible(temperatureEditor);
        addAndMakeVisible(temperatureUnitLabel);
        temperatureUnitLabel.setText(juce::CharPointer_UTF8("\xc2\xb0""C"), juce::dontSendNotification);

        // Master Section
        addAndMakeVisible(masterLevelLabel);
        masterLevelLabel.setText("Master Level:", juce::dontSendNotification);
        addAndMakeVisible(masterLevelEditor);
        addAndMakeVisible(masterLevelUnitLabel);
        masterLevelUnitLabel.setText("dB", juce::dontSendNotification);

        addAndMakeVisible(systemLatencyLabel);
        systemLatencyLabel.setText("System Latency:", juce::dontSendNotification);
        addAndMakeVisible(systemLatencyEditor);
        addAndMakeVisible(systemLatencyUnitLabel);
        systemLatencyUnitLabel.setText("ms", juce::dontSendNotification);

        addAndMakeVisible(haasEffectLabel);
        haasEffectLabel.setText("Haas Effect:", juce::dontSendNotification);
        addAndMakeVisible(haasEffectEditor);
        addAndMakeVisible(haasEffectUnitLabel);
        haasEffectUnitLabel.setText("ms", juce::dontSendNotification);

        // UI Section - Color Scheme
        addAndMakeVisible(colorSchemeLabel);
        colorSchemeLabel.setText("Color Scheme:", juce::dontSendNotification);

        addAndMakeVisible(colorSchemeSelector);
        colorSchemeSelector.addItem("Default (Dark Gray)", 1);
        colorSchemeSelector.addItem("Black", 2);
        colorSchemeSelector.addItem("Light", 3);
        colorSchemeSelector.setSelectedId(ColorScheme::getThemeIndex() + 1, juce::dontSendNotification);
        colorSchemeSelector.onChange = [this]() {
            int schemeIndex = colorSchemeSelector.getSelectedId() - 1;  // Convert to 0-based
            ColorScheme::Manager::getInstance().setTheme(schemeIndex);
            parameters.setConfigParam("ColorScheme", schemeIndex);
        };

        // Store/Reload Section
        addAndMakeVisible(selectProjectFolderButton);
        selectProjectFolderButton.setButtonText("Select Project Folder");
        selectProjectFolderButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF33668C));  // Blueish
        selectProjectFolderButton.onClick = [this]() { selectProjectFolder(); };

        addAndMakeVisible(storeCompleteConfigButton);
        storeCompleteConfigButton.setButtonText("Store Complete Configuration");
        storeCompleteConfigButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF8C3333));  // Reddish
        storeCompleteConfigButton.onClick = [this]() { storeCompleteConfiguration(); };

        addAndMakeVisible(reloadCompleteConfigButton);
        reloadCompleteConfigButton.setButtonText("Reload Complete Configuration");
        reloadCompleteConfigButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF338C33));  // Greenish
        reloadCompleteConfigButton.onClick = [this]() { reloadCompleteConfiguration(); };

        addAndMakeVisible(reloadCompleteConfigBackupButton);
        reloadCompleteConfigBackupButton.setButtonText("Reload Complete Config. Backup");
        reloadCompleteConfigBackupButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF266626));  // Darker green
        reloadCompleteConfigBackupButton.onClick = [this]() { reloadCompleteConfigBackup(); };

        addAndMakeVisible(storeSystemConfigButton);
        storeSystemConfigButton.setButtonText("Store System Configuration");
        storeSystemConfigButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF8C3333));  // Reddish
        storeSystemConfigButton.onClick = [this]() { storeSystemConfiguration(); };

        addAndMakeVisible(reloadSystemConfigButton);
        reloadSystemConfigButton.setButtonText("Reload System Configuration");
        reloadSystemConfigButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF338C33));  // Greenish
        reloadSystemConfigButton.onClick = [this]() { reloadSystemConfiguration(); };

        addAndMakeVisible(reloadSystemConfigBackupButton);
        reloadSystemConfigBackupButton.setButtonText("Reload System Config. Backup");
        reloadSystemConfigBackupButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF266626));  // Darker green
        reloadSystemConfigBackupButton.onClick = [this]() { reloadSystemConfigBackup(); };

        addAndMakeVisible(importSystemConfigButton);
        importSystemConfigButton.setButtonText("Import System Configuration");
        importSystemConfigButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF338C33));  // Greenish
        importSystemConfigButton.onClick = [this]() { importSystemConfiguration(); };

        addAndMakeVisible(exportSystemConfigButton);
        exportSystemConfigButton.setButtonText("Export System Configuration");
        exportSystemConfigButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF8C3333));  // Reddish
        exportSystemConfigButton.onClick = [this]() { exportSystemConfiguration(); };

        // Setup numeric input filtering
        setupNumericEditors();

        // Add text editor listeners
        showNameEditor.addListener(this);
        showLocationEditor.addListener(this);
        inputChannelsEditor.addListener(this);
        outputChannelsEditor.addListener(this);
        reverbChannelsEditor.addListener(this);
        stageWidthEditor.addListener(this);
        stageDepthEditor.addListener(this);
        stageHeightEditor.addListener(this);
        stageOriginWidthEditor.addListener(this);
        stageOriginDepthEditor.addListener(this);
        stageOriginHeightEditor.addListener(this);
        speedOfSoundEditor.addListener(this);
        temperatureEditor.addListener(this);
        masterLevelEditor.addListener(this);
        systemLatencyEditor.addListener(this);
        haasEffectEditor.addListener(this);

        // Listen to parameter changes
        configTree = parameters.getConfigTree();
        configTree.addListener(this);

        // Set explicit tab order: left column first (top-down), then right column (top-down)
        // Left column
        showNameEditor.setExplicitFocusOrder(1);
        showLocationEditor.setExplicitFocusOrder(2);
        inputChannelsEditor.setExplicitFocusOrder(3);
        outputChannelsEditor.setExplicitFocusOrder(4);
        reverbChannelsEditor.setExplicitFocusOrder(5);
        // Right column
        stageWidthEditor.setExplicitFocusOrder(6);
        stageDepthEditor.setExplicitFocusOrder(7);
        stageHeightEditor.setExplicitFocusOrder(8);
        stageOriginWidthEditor.setExplicitFocusOrder(9);
        stageOriginDepthEditor.setExplicitFocusOrder(10);
        stageOriginHeightEditor.setExplicitFocusOrder(11);
        speedOfSoundEditor.setExplicitFocusOrder(12);
        temperatureEditor.setExplicitFocusOrder(13);
        masterLevelEditor.setExplicitFocusOrder(14);
        systemLatencyEditor.setExplicitFocusOrder(15);
        haasEffectEditor.setExplicitFocusOrder(16);

        // Load initial values
        loadParametersToUI();

        // Listen for color scheme changes to update dynamic colors
        ColorScheme::Manager::getInstance().addListener(this);
    }

    ~SystemConfigTab() override
    {
        ColorScheme::Manager::getInstance().removeListener(this);
        configTree.removeListener(this);
    }

    /** ColorScheme::Manager::Listener callback - refresh colors when theme changes */
    void colorSchemeChanged() override
    {
        // Re-apply enabled/disabled colors with current theme colors
        updateIOControlsEnabledState();

        // Update all TextEditor colors - JUCE TextEditors cache colors internally
        // and don't automatically refresh from LookAndFeel on theme change
        const auto& colors = ColorScheme::get();
        auto updateTextEditor = [&colors](juce::TextEditor& editor) {
            editor.setColour(juce::TextEditor::textColourId, colors.textPrimary);
            editor.setColour(juce::TextEditor::backgroundColourId, colors.surfaceCard);
            editor.setColour(juce::TextEditor::outlineColourId, colors.buttonBorder);
            editor.applyFontToAllText(editor.getFont(), true);  // Force text color refresh
        };

        updateTextEditor(showNameEditor);
        updateTextEditor(showLocationEditor);
        updateTextEditor(stageWidthEditor);
        updateTextEditor(stageDepthEditor);
        updateTextEditor(stageHeightEditor);
        updateTextEditor(stageDiameterEditor);
        updateTextEditor(domeElevationEditor);
        updateTextEditor(stageOriginWidthEditor);
        updateTextEditor(stageOriginDepthEditor);
        updateTextEditor(stageOriginHeightEditor);
        updateTextEditor(speedOfSoundEditor);
        updateTextEditor(temperatureEditor);
        updateTextEditor(masterLevelEditor);
        updateTextEditor(systemLatencyEditor);
        updateTextEditor(haasEffectEditor);

        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(ColorScheme::get().background);

        // Footer background (matching Input/Output tabs)
        const int footerHeight = 90;  // Two button rows
        g.setColour(ColorScheme::get().chromeSurface);
        g.fillRect(0, getHeight() - footerHeight, getWidth(), footerHeight);

        // Footer divider line
        g.setColour(ColorScheme::get().chromeDivider);
        g.drawLine(0.0f, (float)(getHeight() - footerHeight), (float)getWidth(), (float)(getHeight() - footerHeight), 1.0f);

        // Section headers (bold like NetworkTab)
        g.setColour(ColorScheme::get().textPrimary);
        g.setFont(juce::FontOptions().withHeight(14.0f).withStyle("Bold"));
        g.drawText("Show", col1X, 10, 200, 20, juce::Justification::left);
        g.drawText("I/O", col1X, 110, 200, 20, juce::Justification::left);
        g.drawText("Stage", col2X, 10, 200, 20, juce::Justification::left);
        g.drawText("Master Section", col2X, 390, 200, 20, juce::Justification::left);
        g.drawText("UI", col3X, 10, 200, 20, juce::Justification::left);
    }

    void resized() override
    {
        const int labelWidth = 150;
        const int editorWidth = 200;
        const int unitWidth = 40;
        const int rowHeight = 30;
        const int spacing = 5;

        int x = col1X;
        int y = 40;

        // Show Section
        showNameLabel.setBounds(x, y, labelWidth, rowHeight);
        showNameEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        showLocationLabel.setBounds(x, y, labelWidth, rowHeight);
        showLocationEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        // I/O Section
        y = 140; // Start after "I/O" header
        inputChannelsLabel.setBounds(x, y, labelWidth, rowHeight);
        inputChannelsEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        outputChannelsLabel.setBounds(x, y, labelWidth, rowHeight);
        outputChannelsEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        reverbChannelsLabel.setBounds(x, y, labelWidth, rowHeight);
        reverbChannelsEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        audioPatchingButton.setBounds(x, y, editorWidth + labelWidth, rowHeight);
        y += rowHeight + spacing;

        algorithmLabel.setBounds(x, y, labelWidth, rowHeight);
        algorithmSelector.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        processingButton.setBounds(x, y, editorWidth + labelWidth, rowHeight);

        // Stage Section
        x = col2X;
        y = 40;

        // Stage shape selector (same width as number editors)
        stageShapeLabel.setBounds(x, y, labelWidth, rowHeight);
        stageShapeSelector.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        // Dimension row 1: Width (box) or Diameter (cylinder/dome)
        stageWidthLabel.setBounds(x, y, labelWidth, rowHeight);
        stageWidthEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        stageWidthUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        stageDiameterLabel.setBounds(x, y, labelWidth, rowHeight);
        stageDiameterEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        stageDiameterUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        // Dimension row 2: Depth (box) or Height (cylinder) or Elevation (dome)
        stageDepthLabel.setBounds(x, y, labelWidth, rowHeight);
        stageDepthEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        stageDepthUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        domeElevationLabel.setBounds(x, y, labelWidth, rowHeight);
        domeElevationEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        domeElevationUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        // Dimension row 3: Height (box/cylinder only)
        stageHeightLabel.setBounds(x, y, labelWidth, rowHeight);
        stageHeightEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        stageHeightUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        // Origin coordinates with preset buttons to the right of each row
        const int originButtonSize = 30;  // Square buttons sized to match row height
        int buttonX = x + labelWidth + editorWidth + spacing + unitWidth + spacing;

        stageOriginWidthLabel.setBounds(x, y, labelWidth, rowHeight);
        stageOriginWidthEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        stageOriginWidthUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        originFrontButton.setBounds(buttonX, y, originButtonSize, rowHeight);
        y += rowHeight + spacing;

        stageOriginDepthLabel.setBounds(x, y, labelWidth, rowHeight);
        stageOriginDepthEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        stageOriginDepthUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        originCenterGroundButton.setBounds(buttonX, y, originButtonSize, rowHeight);
        y += rowHeight + spacing;

        stageOriginHeightLabel.setBounds(x, y, labelWidth, rowHeight);
        stageOriginHeightEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        stageOriginHeightUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        originCenterButton.setBounds(buttonX, y, originButtonSize, rowHeight);
        y += rowHeight + spacing;

        speedOfSoundLabel.setBounds(x, y, labelWidth, rowHeight);
        speedOfSoundEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        speedOfSoundUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        temperatureLabel.setBounds(x, y, labelWidth, rowHeight);
        temperatureEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        temperatureUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);

        // Master Section
        x = col2X;
        y = 420;

        masterLevelLabel.setBounds(x, y, labelWidth, rowHeight);
        masterLevelEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        masterLevelUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        systemLatencyLabel.setBounds(x, y, labelWidth, rowHeight);
        systemLatencyEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        systemLatencyUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        haasEffectLabel.setBounds(x, y, labelWidth, rowHeight);
        haasEffectEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        haasEffectUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);

        // UI Section (third column)
        x = col3X;
        y = 40;

        colorSchemeLabel.setBounds(x, y, labelWidth, rowHeight);
        colorSchemeSelector.setBounds(x + labelWidth, y, editorWidth, rowHeight);

        // Footer buttons - full width at bottom (matching Output tab style)
        const int footerHeight = 90;  // Two 30px button rows + 10px spacing + 20px padding
        const int footerPadding = 10;
        const int buttonRowHeight = 30;  // Same as Output tab buttons
        auto footerArea = getLocalBounds().removeFromBottom(footerHeight).reduced(footerPadding, footerPadding);

        // Row 1: Project folder + Complete config buttons - 4 items
        auto row1 = footerArea.removeFromTop(buttonRowHeight);
        const int row1ButtonWidth = (row1.getWidth() - spacing * 3) / 4;

        selectProjectFolderButton.setBounds(row1.removeFromLeft(row1ButtonWidth));
        row1.removeFromLeft(spacing);
        storeCompleteConfigButton.setBounds(row1.removeFromLeft(row1ButtonWidth));
        row1.removeFromLeft(spacing);
        reloadCompleteConfigButton.setBounds(row1.removeFromLeft(row1ButtonWidth));
        row1.removeFromLeft(spacing);
        reloadCompleteConfigBackupButton.setBounds(row1);

        footerArea.removeFromTop(footerPadding);  // Same spacing as padding for consistency

        // Row 2: System config buttons - 5 equal-width buttons
        auto row2 = footerArea.removeFromTop(buttonRowHeight);
        const int sysButtonWidth = (row2.getWidth() - spacing * 4) / 5;

        storeSystemConfigButton.setBounds(row2.removeFromLeft(sysButtonWidth));
        row2.removeFromLeft(spacing);
        reloadSystemConfigButton.setBounds(row2.removeFromLeft(sysButtonWidth));
        row2.removeFromLeft(spacing);
        reloadSystemConfigBackupButton.setBounds(row2.removeFromLeft(sysButtonWidth));
        row2.removeFromLeft(spacing);
        importSystemConfigButton.setBounds(row2.removeFromLeft(sysButtonWidth));
        row2.removeFromLeft(spacing);
        exportSystemConfigButton.setBounds(row2);
    }

    void setStatusBar(StatusBar* bar)
    {
        statusBar = bar;
        setupHelpText();
        setupMouseListeners();
    }

    void setProcessingCallback(ProcessingCallback callback)
    {
        onProcessingChanged = callback;
    }

    void setChannelCountCallback(ChannelCountCallback callback)
    {
        onChannelCountChanged = callback;
    }

    void setAudioInterfaceCallback(AudioInterfaceCallback callback)
    {
        onAudioInterfaceWindowRequested = callback;
    }

    void setConfigReloadedCallback(ConfigReloadedCallback callback)
    {
        onConfigReloaded = callback;
    }

    /** Grab focus when this tab becomes visible to prevent auto-focus on first TextEditor */
    void visibilityChanged() override
    {
        // Only grab focus if we're actually on screen (isShowing checks for peer)
        if (isShowing())
            grabKeyboardFocus();
    }

    /** Grab focus when clicking on background (not on a child component) */
    void mouseDown(const juce::MouseEvent& e) override
    {
        // Only grab focus if clicking directly on this component (not a child)
        if (e.eventComponent == this)
            grabKeyboardFocus();
    }

private:
    //==============================================================================
    // Helper methods

    void setupNumericEditors()
    {
        auto setupNumericEditor = [](juce::TextEditor& editor, bool allowNegative, bool allowDecimal) {
            juce::String allowedChars = "0123456789";
            if (allowNegative) allowedChars += "-";
            if (allowDecimal) allowedChars += ".";
            editor.setInputFilter(new juce::TextEditor::LengthAndCharacterRestriction(10, allowedChars), true);
            editor.setPopupMenuEnabled(false);
            editor.setSelectAllWhenFocused(true);
        };

        // Stage Section - floats, no negatives (take absolute value)
        setupNumericEditor(stageWidthEditor, false, true);  // 0.0 to 100.0
        setupNumericEditor(stageDepthEditor, false, true);  // 0.0 to 100.0
        setupNumericEditor(stageHeightEditor, false, true);  // 0.0 to 100.0

        // Stage Origin - floats, allows negatives
        setupNumericEditor(stageOriginWidthEditor, true, true);  // -100.0 to 200.0
        setupNumericEditor(stageOriginDepthEditor, true, true);  // -100.0 to 200.0
        setupNumericEditor(stageOriginHeightEditor, true, true);  // -100.0 to 200.0

        // Speed of Sound - float, no negative (take absolute value)
        setupNumericEditor(speedOfSoundEditor, false, true);  // 319.2 to 367.7

        // Temperature - float, allows negative
        setupNumericEditor(temperatureEditor, true, true);  // -20.0 to 60.0

        // Master Section
        setupNumericEditor(masterLevelEditor, true, true);  // -92.0 to 0.0 (allows negative)
        setupNumericEditor(systemLatencyEditor, false, true);  // 0.0 to 10.0 (no negative)
        setupNumericEditor(haasEffectEditor, false, true);  // 0.0 to 10.0 (no negative)

        // I/O Section - integers only
        setupNumericEditor(inputChannelsEditor, false, false);
        setupNumericEditor(outputChannelsEditor, false, false);
        setupNumericEditor(reverbChannelsEditor, false, false);
    }

    //==============================================================================
    // TextEditor::Listener callbacks

    void textEditorTextChanged(juce::TextEditor&) override
    {
        // Don't update parameters during typing - only on Enter or focus lost
    }

    void textEditorReturnKeyPressed(juce::TextEditor& editor) override
    {
        // Set flag to prevent textEditorFocusLost from re-validating
        isValidatingFromReturnKey = true;
        validateAndClampValue(editor);
        editor.giveAwayKeyboardFocus();
        isValidatingFromReturnKey = false;
    }

    void textEditorEscapeKeyPressed(juce::TextEditor& editor) override
    {
        // Restore original value from parameters without updating
        if (&editor == &showNameEditor)
            editor.setText(parameters.getConfigParam("ShowName").toString(), false);
        else if (&editor == &showLocationEditor)
            editor.setText(parameters.getConfigParam("ShowLocation").toString(), false);
        else if (&editor == &inputChannelsEditor)
            editor.setText(juce::String(parameters.getNumInputChannels()), false);
        else if (&editor == &outputChannelsEditor)
            editor.setText(juce::String(parameters.getNumOutputChannels()), false);
        else if (&editor == &reverbChannelsEditor)
            editor.setText(juce::String(parameters.getNumReverbChannels()), false);
        else if (&editor == &stageWidthEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("StageWidth"), 2), false);
        else if (&editor == &stageDepthEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("StageDepth"), 2), false);
        else if (&editor == &stageHeightEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("StageHeight"), 2), false);
        else if (&editor == &stageDiameterEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("StageDiameter"), 2), false);
        else if (&editor == &domeElevationEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("DomeElevation"), 0), false);
        else if (&editor == &stageOriginWidthEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("StageOriginWidth"), 2), false);
        else if (&editor == &stageOriginDepthEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("StageOriginDepth"), 2), false);
        else if (&editor == &stageOriginHeightEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("StageOriginHeight"), 2), false);
        else if (&editor == &speedOfSoundEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("SpeedOfSound"), 2), false);
        else if (&editor == &temperatureEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("Temperature"), 2), false);
        else if (&editor == &masterLevelEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("MasterLevel"), 2), false);
        else if (&editor == &systemLatencyEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("SystemLatency"), 2), false);
        else if (&editor == &haasEffectEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("HaasEffect"), 2), false);

        editor.giveAwayKeyboardFocus();
    }

    void textEditorFocusLost(juce::TextEditor& editor) override
    {
        // Skip if we just validated via Return key (prevents double dialog)
        if (isValidatingFromReturnKey)
            return;
        validateAndClampValue(editor);
    }

    //==============================================================================
    // ValueTree::Listener callbacks

    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override
    {
        loadParametersToUI();
    }

    //==============================================================================
    // Parameter management

    void loadParametersToUI()
    {
        // String values
        showNameEditor.setText(parameters.getConfigParam("ShowName").toString(), false);
        showLocationEditor.setText(parameters.getConfigParam("ShowLocation").toString(), false);

        // Channel counts - use dedicated getters for reliable values
        inputChannelsEditor.setText(juce::String(parameters.getNumInputChannels()), false);
        outputChannelsEditor.setText(juce::String(parameters.getNumOutputChannels()), false);
        reverbChannelsEditor.setText(juce::String(parameters.getNumReverbChannels()), false);

        // Stage shape selector
        int shapeId = (int)parameters.getConfigParam("StageShape");
        stageShapeSelector.setSelectedId(shapeId + 1, juce::dontSendNotification);  // +1 because ComboBox IDs start at 1

        // Float values - format with 2 decimal places to avoid precision issues
        stageWidthEditor.setText(juce::String((float)parameters.getConfigParam("StageWidth"), 2), false);
        stageDepthEditor.setText(juce::String((float)parameters.getConfigParam("StageDepth"), 2), false);
        stageHeightEditor.setText(juce::String((float)parameters.getConfigParam("StageHeight"), 2), false);
        stageDiameterEditor.setText(juce::String((float)parameters.getConfigParam("StageDiameter"), 2), false);
        domeElevationEditor.setText(juce::String((float)parameters.getConfigParam("DomeElevation"), 0), false);
        stageOriginWidthEditor.setText(juce::String((float)parameters.getConfigParam("StageOriginWidth"), 2), false);
        stageOriginDepthEditor.setText(juce::String((float)parameters.getConfigParam("StageOriginDepth"), 2), false);
        stageOriginHeightEditor.setText(juce::String((float)parameters.getConfigParam("StageOriginHeight"), 2), false);
        speedOfSoundEditor.setText(juce::String((float)parameters.getConfigParam("SpeedOfSound"), 2), false);
        temperatureEditor.setText(juce::String((float)parameters.getConfigParam("Temperature"), 2), false);

        // Update visibility based on shape
        updateStageParameterVisibility();
        masterLevelEditor.setText(juce::String((float)parameters.getConfigParam("MasterLevel"), 2), false);
        systemLatencyEditor.setText(juce::String((float)parameters.getConfigParam("SystemLatency"), 2), false);
        haasEffectEditor.setText(juce::String((float)parameters.getConfigParam("HaasEffect"), 2), false);

        // Algorithm selector
        int algorithmId = (int)parameters.getConfigParam("ProcessingAlgorithm");
        if (algorithmId >= 1 && algorithmId <= 2)  // Valid range for current algorithms
            algorithmSelector.setSelectedId(algorithmId, juce::dontSendNotification);

        // Color scheme selector - update UI and apply the theme
        int colorSchemeId = (int)parameters.getConfigParam("ColorScheme");
        colorSchemeSelector.setSelectedId(colorSchemeId + 1, juce::dontSendNotification);  // +1 for ComboBox
        ColorScheme::Manager::getInstance().setTheme(colorSchemeId);  // Actually apply the theme

        // Processing button state
        processingEnabled = (bool)parameters.getConfigParam("ProcessingEnabled");
        processingButton.setButtonText(processingEnabled ? "Processing: ON" : "Processing: OFF");

        // Update I/O controls enabled state based on processing state
        updateIOControlsEnabledState();
    }

    void updateParameterFromEditor(juce::TextEditor& editor)
    {
        auto text = editor.getText();

        if (&editor == &showNameEditor)
            parameters.setConfigParam("ShowName", text);
        else if (&editor == &showLocationEditor)
            parameters.setConfigParam("ShowLocation", text);
        else if (&editor == &inputChannelsEditor)
        {
            int newInputs = text.getIntValue();
            int currentInputs = parameters.getNumInputChannels();
            if (newInputs < currentInputs)
            {
                // Prevent multiple dialogs from appearing
                if (isShowingChannelReductionDialog)
                {
                    inputChannelsEditor.setText(juce::String(currentInputs), false);
                    return;
                }
                isShowingChannelReductionDialog = true;

                // Show JUCE AlertWindow confirmation dialog for reducing channels
                auto options = juce::MessageBoxOptions()
                    .withIconType(juce::MessageBoxIconType::WarningIcon)
                    .withTitle("Reduce Input Channels?")
                    .withMessage("Reducing from " + juce::String(currentInputs) + " to " + juce::String(newInputs) +
                        " input channels will remove settings for channels " + juce::String(newInputs + 1) +
                        " to " + juce::String(currentInputs) + ".\n\nThis cannot be undone.")
                    .withButton("Reduce")
                    .withButton("Cancel")
                    .withAssociatedComponent(this);

                juce::AlertWindow::showAsync(options, [this, newInputs, currentInputs](int result) {
                    isShowingChannelReductionDialog = false;
                    if (result == 1)  // Reduce pressed
                    {
                        parameters.setNumInputChannels(newInputs);
                        notifyChannelCountChanged();
                    }
                    else  // Cancel pressed - restore original value
                    {
                        inputChannelsEditor.setText(juce::String(currentInputs), false);
                    }
                });
            }
            else
            {
                parameters.setNumInputChannels(newInputs);
                notifyChannelCountChanged();
            }
        }
        else if (&editor == &outputChannelsEditor)
        {
            int newOutputs = text.getIntValue();
            int currentOutputs = parameters.getNumOutputChannels();
            if (newOutputs < currentOutputs)
            {
                // Prevent multiple dialogs from appearing
                if (isShowingChannelReductionDialog)
                {
                    outputChannelsEditor.setText(juce::String(currentOutputs), false);
                    return;
                }
                isShowingChannelReductionDialog = true;

                // Show JUCE AlertWindow confirmation dialog for reducing channels
                auto options = juce::MessageBoxOptions()
                    .withIconType(juce::MessageBoxIconType::WarningIcon)
                    .withTitle("Reduce Output Channels?")
                    .withMessage("Reducing from " + juce::String(currentOutputs) + " to " + juce::String(newOutputs) +
                        " output channels will remove settings for channels " + juce::String(newOutputs + 1) +
                        " to " + juce::String(currentOutputs) + ".\n\nThis cannot be undone.")
                    .withButton("Reduce")
                    .withButton("Cancel")
                    .withAssociatedComponent(this);

                juce::AlertWindow::showAsync(options, [this, newOutputs, currentOutputs](int result) {
                    isShowingChannelReductionDialog = false;
                    if (result == 1)  // Reduce pressed
                    {
                        parameters.setNumOutputChannels(newOutputs);
                        notifyChannelCountChanged();
                    }
                    else  // Cancel pressed - restore original value
                    {
                        outputChannelsEditor.setText(juce::String(currentOutputs), false);
                    }
                });
            }
            else
            {
                parameters.setNumOutputChannels(newOutputs);
                notifyChannelCountChanged();
            }
        }
        else if (&editor == &reverbChannelsEditor)
        {
            int newReverbs = text.getIntValue();
            int currentReverbs = parameters.getNumReverbChannels();
            if (newReverbs < currentReverbs)
            {
                // Prevent multiple dialogs from appearing
                if (isShowingChannelReductionDialog)
                {
                    reverbChannelsEditor.setText(juce::String(currentReverbs), false);
                    return;
                }
                isShowingChannelReductionDialog = true;

                // Show JUCE AlertWindow confirmation dialog for reducing channels
                auto options = juce::MessageBoxOptions()
                    .withIconType(juce::MessageBoxIconType::WarningIcon)
                    .withTitle("Reduce Reverb Channels?")
                    .withMessage("Reducing from " + juce::String(currentReverbs) + " to " + juce::String(newReverbs) +
                        " reverb channels will remove settings for channels " + juce::String(newReverbs + 1) +
                        " to " + juce::String(currentReverbs) + ".\n\nThis cannot be undone.")
                    .withButton("Reduce")
                    .withButton("Cancel")
                    .withAssociatedComponent(this);

                juce::AlertWindow::showAsync(options, [this, newReverbs, currentReverbs](int result) {
                    isShowingChannelReductionDialog = false;
                    if (result == 1)  // Reduce pressed
                    {
                        parameters.setNumReverbChannels(newReverbs);
                        notifyChannelCountChanged();
                    }
                    else  // Cancel pressed - restore original value
                    {
                        reverbChannelsEditor.setText(juce::String(currentReverbs), false);
                    }
                });
            }
            else
            {
                parameters.setNumReverbChannels(newReverbs);
                notifyChannelCountChanged();
            }
        }
        else if (&editor == &stageWidthEditor)
            parameters.setConfigParam("StageWidth", text.getFloatValue());
        else if (&editor == &stageDepthEditor)
            parameters.setConfigParam("StageDepth", text.getFloatValue());
        else if (&editor == &stageHeightEditor)
            parameters.setConfigParam("StageHeight", text.getFloatValue());
        else if (&editor == &stageDiameterEditor)
            parameters.setConfigParam("StageDiameter", text.getFloatValue());
        else if (&editor == &domeElevationEditor)
            parameters.setConfigParam("DomeElevation", text.getFloatValue());
        else if (&editor == &stageOriginWidthEditor)
            parameters.setConfigParam("StageOriginWidth", text.getFloatValue());
        else if (&editor == &stageOriginDepthEditor)
            parameters.setConfigParam("StageOriginDepth", text.getFloatValue());
        else if (&editor == &stageOriginHeightEditor)
            parameters.setConfigParam("StageOriginHeight", text.getFloatValue());
        else if (&editor == &speedOfSoundEditor)
        {
            float speedOfSound = text.getFloatValue();
            parameters.setConfigParam("SpeedOfSound", speedOfSound);
            // Calculate and update temperature: T = (c - 331.3) / 0.606
            float temperature = (speedOfSound - 331.3f) / 0.606f;
            temperature = juce::jlimit(-20.0f, 60.0f, temperature);
            parameters.setConfigParam("Temperature", temperature);
        }
        else if (&editor == &temperatureEditor)
        {
            float temperature = text.getFloatValue();
            parameters.setConfigParam("Temperature", temperature);
            // Calculate and update speed of sound: c = 331.3 + 0.606 * T
            float speedOfSound = 331.3f + 0.606f * temperature;
            speedOfSound = juce::jlimit(319.2f, 367.7f, speedOfSound);
            parameters.setConfigParam("SpeedOfSound", speedOfSound);
        }
        else if (&editor == &masterLevelEditor)
            parameters.setConfigParam("MasterLevel", text.getFloatValue());
        else if (&editor == &systemLatencyEditor)
            parameters.setConfigParam("SystemLatency", text.getFloatValue());
        else if (&editor == &haasEffectEditor)
            parameters.setConfigParam("HaasEffect", text.getFloatValue());
    }

    void validateAndClampValue(juce::TextEditor& editor)
    {
        auto text = editor.getText();

        // String fields - just update parameter, no validation needed
        if (&editor == &showNameEditor || &editor == &showLocationEditor)
        {
            updateParameterFromEditor(editor);
            return;
        }

        // If empty, restore default
        if (text.isEmpty())
        {
            loadParametersToUI();
            return;
        }

        float value = text.getFloatValue();

        // Validate and clamp based on CSV specifications
        // Take absolute value for fields that don't allow negatives
        if (&editor == &stageWidthEditor)
            value = juce::jlimit(0.0f, 100.0f, std::abs(value));
        else if (&editor == &stageDepthEditor)
            value = juce::jlimit(0.0f, 100.0f, std::abs(value));
        else if (&editor == &stageHeightEditor)
            value = juce::jlimit(0.0f, 100.0f, std::abs(value));
        else if (&editor == &stageDiameterEditor)
            value = juce::jlimit(0.1f, 100.0f, std::abs(value));  // Min 0.1m, max 100m
        else if (&editor == &domeElevationEditor)
            value = juce::jlimit(1.0f, 360.0f, std::abs(value));  // Min 1°, max 360°
        else if (&editor == &stageOriginWidthEditor)
            value = juce::jlimit(-100.0f, 200.0f, value);  // Allows negative
        else if (&editor == &stageOriginDepthEditor)
            value = juce::jlimit(-100.0f, 200.0f, value);  // Allows negative
        else if (&editor == &stageOriginHeightEditor)
            value = juce::jlimit(-100.0f, 200.0f, value);  // Allows negative
        else if (&editor == &speedOfSoundEditor)
            value = juce::jlimit(319.2f, 367.7f, std::abs(value));
        else if (&editor == &temperatureEditor)
            value = juce::jlimit(-20.0f, 60.0f, value);  // Allows negative
        else if (&editor == &masterLevelEditor)
            value = juce::jlimit(-92.0f, 0.0f, value);  // Allows negative
        else if (&editor == &systemLatencyEditor)
            value = juce::jlimit(0.0f, 10.0f, std::abs(value));
        else if (&editor == &haasEffectEditor)
            value = juce::jlimit(0.0f, 10.0f, std::abs(value));

        // Update display with clamped value (2 decimal places for floats)
        if (&editor == &inputChannelsEditor || &editor == &outputChannelsEditor ||
            &editor == &reverbChannelsEditor)
        {
            editor.setText(juce::String((int)value), false);
        }
        else
        {
            editor.setText(juce::String(value, 2), false);
        }

        // Update parameter
        updateParameterFromEditor(editor);
    }

    //==============================================================================
    // Processing toggle

    void toggleProcessing()
    {
        processingEnabled = !processingEnabled;
        processingButton.setButtonText(processingEnabled ? "Processing: ON" : "Processing: OFF");
        parameters.setConfigParam("ProcessingEnabled", processingEnabled);

        // Lock/unlock I/O controls based on processing state
        updateIOControlsEnabledState();

        // Notify MainComponent of processing state change
        if (onProcessingChanged)
            onProcessingChanged(processingEnabled);
    }

    void updateIOControlsEnabledState()
    {
        // When processing is ON, disable I/O controls to prevent changes
        bool enabled = !processingEnabled;

        inputChannelsEditor.setEnabled(enabled);
        outputChannelsEditor.setEnabled(enabled);
        reverbChannelsEditor.setEnabled(enabled);
        audioPatchingButton.setEnabled(enabled);
        algorithmSelector.setEnabled(enabled);

        // Visual feedback - dim disabled controls using theme colors
        auto disabledColour = ColorScheme::get().textDisabled;
        auto enabledColour = ColorScheme::get().textPrimary;

        inputChannelsEditor.setColour(juce::TextEditor::textColourId, enabled ? enabledColour : disabledColour);
        outputChannelsEditor.setColour(juce::TextEditor::textColourId, enabled ? enabledColour : disabledColour);
        reverbChannelsEditor.setColour(juce::TextEditor::textColourId, enabled ? enabledColour : disabledColour);
    }

    //==============================================================================
    // Stage shape methods

    void onStageShapeChanged()
    {
        int shapeId = stageShapeSelector.getSelectedId() - 1;  // Convert to 0-based
        parameters.setConfigParam("StageShape", shapeId);

        // Reset dimensions to defaults when shape changes
        if (shapeId == 0)  // Box
        {
            parameters.setConfigParam("StageWidth", WFSParameterDefaults::stageWidthDefault);
            parameters.setConfigParam("StageDepth", WFSParameterDefaults::stageDepthDefault);
            parameters.setConfigParam("StageHeight", WFSParameterDefaults::stageHeightDefault);
        }
        else if (shapeId == 1)  // Cylinder
        {
            parameters.setConfigParam("StageDiameter", WFSParameterDefaults::stageDiameterDefault);
            parameters.setConfigParam("StageHeight", WFSParameterDefaults::stageHeightDefault);
        }
        else  // Dome
        {
            parameters.setConfigParam("StageDiameter", WFSParameterDefaults::stageDiameterDefault);
            parameters.setConfigParam("DomeElevation", WFSParameterDefaults::domeElevationDefault);
        }

        // Reset origin to center
        setOriginToCenterGround();

        // Update UI
        loadParametersToUI();
    }

    void updateStageParameterVisibility()
    {
        int shapeId = stageShapeSelector.getSelectedId() - 1;  // 0=box, 1=cylinder, 2=dome

        // Box: show width, depth, height
        bool isBox = (shapeId == 0);
        stageWidthLabel.setVisible(isBox);
        stageWidthEditor.setVisible(isBox);
        stageWidthUnitLabel.setVisible(isBox);
        stageDepthLabel.setVisible(isBox);
        stageDepthEditor.setVisible(isBox);
        stageDepthUnitLabel.setVisible(isBox);

        // Cylinder/Dome: show diameter
        bool isCircular = (shapeId != 0);
        stageDiameterLabel.setVisible(isCircular);
        stageDiameterEditor.setVisible(isCircular);
        stageDiameterUnitLabel.setVisible(isCircular);

        // Box/Cylinder: show height
        bool hasHeight = (shapeId == 0 || shapeId == 1);
        stageHeightLabel.setVisible(hasHeight);
        stageHeightEditor.setVisible(hasHeight);
        stageHeightUnitLabel.setVisible(hasHeight);

        // Dome: show elevation
        bool isDome = (shapeId == 2);
        domeElevationLabel.setVisible(isDome);
        domeElevationEditor.setVisible(isDome);
        domeElevationUnitLabel.setVisible(isDome);
    }

    //==============================================================================
    // Origin preset methods

    void setOriginToFront()
    {
        // Front-center of stage: X = center, Y = front edge, Z = floor
        int shapeId = stageShapeSelector.getSelectedId() - 1;
        float frontOffset = 0.0f;

        if (shapeId == 0)  // Box
            frontOffset = -((float)parameters.getConfigParam("StageDepth")) * 0.5f;
        else  // Cylinder/Dome
            frontOffset = -((float)parameters.getConfigParam("StageDiameter")) * 0.5f;

        parameters.setConfigParam("StageOriginWidth", 0.0f);
        parameters.setConfigParam("StageOriginDepth", frontOffset);
        parameters.setConfigParam("StageOriginHeight", 0.0f);
    }

    void setOriginToCenterGround()
    {
        // Center at ground level: all offsets zero (center-referenced)
        parameters.setConfigParam("StageOriginWidth", 0.0f);
        parameters.setConfigParam("StageOriginDepth", 0.0f);
        parameters.setConfigParam("StageOriginHeight", 0.0f);
    }

    void setOriginToCenter()
    {
        // Center of stage cube/cylinder/dome: X = center, Y = center, Z = half height
        int shapeId = stageShapeSelector.getSelectedId() - 1;
        float halfHeight = 0.0f;

        if (shapeId == 2)  // Dome - use diameter for spherical center
            halfHeight = ((float)parameters.getConfigParam("StageDiameter")) * 0.5f;
        else  // Box/Cylinder
            halfHeight = ((float)parameters.getConfigParam("StageHeight")) * 0.5f;

        parameters.setConfigParam("StageOriginWidth", 0.0f);
        parameters.setConfigParam("StageOriginDepth", 0.0f);
        parameters.setConfigParam("StageOriginHeight", halfHeight);
    }

    //==============================================================================
    // Store/Reload methods

    void selectProjectFolder()
    {
        auto chooser = std::make_shared<juce::FileChooser>("Select Project Folder", projectFolder);
        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;

        chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.exists() && result.isDirectory())
            {
                projectFolder = result;
                parameters.setConfigParam("ProjectFolder", projectFolder.getFullPathName());
                parameters.getFileManager().setProjectFolder(projectFolder);
            }
        });
    }

    void storeCompleteConfiguration()
    {
        auto& fileManager = parameters.getFileManager();

        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage("Please select a project folder first.");
            return;
        }

        // Backup is created automatically by file manager before overwrite
        if (fileManager.saveCompleteConfig())
            showStatusMessage("Complete configuration saved.");
        else
            showStatusMessage("Error: " + fileManager.getLastError());
    }

    void reloadCompleteConfiguration()
    {
        auto& fileManager = parameters.getFileManager();

        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage("Please select a project folder first.");
            return;
        }

        // Load complete config from individual files (system.xml, network.xml, inputs.xml, outputs.xml, reverbs.xml)
        bool success = fileManager.loadCompleteConfig();

        if (success)
            showStatusMessage("Complete configuration loaded.");
        else
            showStatusMessage("Partial load: " + fileManager.getLastError());

        // Always refresh UI and trigger recalculation, even on partial load
        // Some files may have loaded successfully (e.g., system and outputs but not reverbs)

        // Notify MainComponent of channel count changes to refresh UI
        notifyChannelCountChanged();

        // Refresh the UI to show loaded values
        loadParametersToUI();

        // Notify MainComponent to refresh all tabs
        if (onConfigReloaded)
            onConfigReloaded();
    }

    void reloadCompleteConfigBackup()
    {
        auto& fileManager = parameters.getFileManager();

        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage("Please select a project folder first.");
            return;
        }

        auto backups = fileManager.getBackups("complete");
        if (backups.isEmpty())
        {
            showStatusMessage("No backup files found.");
            return;
        }

        bool success = fileManager.loadCompleteConfigBackup(0);

        if (success)
            showStatusMessage("Configuration loaded from backup.");
        else
            showStatusMessage("Partial load from backup: " + fileManager.getLastError());

        // Always refresh UI and trigger recalculation, even on partial load

        // Notify MainComponent of channel count changes to refresh UI
        notifyChannelCountChanged();

        // Refresh the UI to show loaded values
        loadParametersToUI();

        // Notify MainComponent to refresh all tabs
        if (onConfigReloaded)
            onConfigReloaded();
    }

    void storeSystemConfiguration()
    {
        auto& fileManager = parameters.getFileManager();

        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage("Please select a project folder first.");
            return;
        }

        // Backup is created automatically by file manager before overwrite
        if (fileManager.saveSystemConfig())
            showStatusMessage("System configuration saved.");
        else
            showStatusMessage("Error: " + fileManager.getLastError());
    }

    void reloadSystemConfiguration()
    {
        auto& fileManager = parameters.getFileManager();

        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage("Please select a project folder first.");
            return;
        }

        auto configFile = fileManager.getSystemConfigFile();
        if (!configFile.existsAsFile())
        {
            showStatusMessage("System configuration file not found.");
            return;
        }

        if (fileManager.loadSystemConfig())
        {
            showStatusMessage("System configuration loaded.");

            // Update UI from ValueTree
            loadParametersToUI();

            // Notify MainComponent to refresh all tabs
            if (onConfigReloaded)
                onConfigReloaded();
        }
        else
        {
            showStatusMessage("Error: " + fileManager.getLastError());
        }
    }

    void reloadSystemConfigBackup()
    {
        auto& fileManager = parameters.getFileManager();

        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage("Please select a project folder first.");
            return;
        }

        auto backups = fileManager.getBackups("system");
        if (backups.isEmpty())
        {
            showStatusMessage("No backup files found.");
            return;
        }

        if (fileManager.loadSystemConfigBackup(0))
        {
            showStatusMessage("System configuration loaded from backup.");

            // Update UI from ValueTree
            loadParametersToUI();

            // Notify MainComponent to refresh all tabs
            if (onConfigReloaded)
                onConfigReloaded();
        }
        else
        {
            showStatusMessage("Error: " + fileManager.getLastError());
        }
    }

    void importSystemConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser>("Import System Configuration",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.xml");
        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.existsAsFile())
            {
                auto& fileManager = parameters.getFileManager();
                if (fileManager.importSystemConfig(result))
                {
                    showStatusMessage("System configuration imported.");

                    // Update UI from ValueTree
                    loadParametersToUI();

                    // Notify MainComponent to refresh all tabs
                    if (onConfigReloaded)
                        onConfigReloaded();
                }
                else
                {
                    showStatusMessage("Error: " + fileManager.getLastError());
                }
            }
        });
    }

    void exportSystemConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser>("Export System Configuration",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.xml");
        auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result != juce::File())
            {
                // Add .xml extension if not present
                if (!result.hasFileExtension(".xml"))
                    result = result.withFileExtension(".xml");

                auto& fileManager = parameters.getFileManager();
                if (fileManager.exportSystemConfig(result))
                    showStatusMessage("System configuration exported.");
                else
                    showStatusMessage("Error: " + fileManager.getLastError());
            }
        });
    }

    //==============================================================================
    // Status bar helper methods

    void showStatusMessage(const juce::String& message, int durationMs = 3000)
    {
        if (statusBar != nullptr)
            statusBar->showTemporaryMessage(message, durationMs);
    }

    void setupHelpText()
    {
        // Based on WFS-UI_config.csv help text column
        helpTextMap[&showNameEditor] = "Name of the current show.";
        helpTextMap[&showLocationEditor] = "Location of the current show.";
        helpTextMap[&inputChannelsEditor] = "Number of Input Channels.";
        helpTextMap[&outputChannelsEditor] = "Number of Output Channels.";
        helpTextMap[&reverbChannelsEditor] = "Number of Reverb Channels.";
        helpTextMap[&audioPatchingButton] = "Open patching window to route Input and Output channels to the Audio Interface.";
        helpTextMap[&algorithmSelector] = "Select the rendering algorithm from the menu.";
        helpTextMap[&processingButton] = "Lock all I/O parameters and start the DSP. Long press to stop the DSP.";
        helpTextMap[&stageShapeSelector] = "Stage shape: Box (rectangular), Cylinder (circular footprint), or Dome (partial sphere).";
        helpTextMap[&stageWidthEditor] = "Width of the stage in meters (Box shape only).";
        helpTextMap[&stageDepthEditor] = "Depth of the stage in meters (Box shape only).";
        helpTextMap[&stageHeightEditor] = "Height of the stage in meters (Box and Cylinder shapes).";
        helpTextMap[&stageDiameterEditor] = "Diameter of the stage in meters (Cylinder and Dome shapes).";
        helpTextMap[&domeElevationEditor] = "Dome elevation angle: 180 = hemisphere, 360 = full sphere.";
        helpTextMap[&stageOriginWidthEditor] = "Origin X offset from stage center (0 = centered, negative = left).";
        helpTextMap[&stageOriginDepthEditor] = "Origin Y offset from stage center (0 = centered, negative = front/downstage).";
        helpTextMap[&stageOriginHeightEditor] = "Origin Z offset from floor (0 = floor level, positive = above floor).";
        helpTextMap[&originFrontButton] = "Set origin to front center of stage. Typical for frontal stages.";
        helpTextMap[&originCenterGroundButton] = "Set origin to center of stage at ground level. Typical for Surround or Cylindrical setups.";
        helpTextMap[&originCenterButton] = "Set origin to center of stage volume. Typical for Spherical Dome setups.";
        helpTextMap[&speedOfSoundEditor] = "Speed of Sound (related to the temperature).";
        helpTextMap[&temperatureEditor] = "Temperature (gives the Speed of Sound).";
        helpTextMap[&masterLevelEditor] = "Master Level (affects all outputs).";
        helpTextMap[&systemLatencyEditor] = "Total latency of the system (Mixing board & Computer) / Specific Input and Output Latency/Delay can be set in the respective Input and Output settings.";
        helpTextMap[&haasEffectEditor] = "Hass Effect to apply to the system. Will take into account the Latency Compensations (System, Input and Output).";
        helpTextMap[&colorSchemeSelector] = "Select the color scheme: Default (dark gray), Black (pure black for OLED displays), or Light (daytime use).";
        helpTextMap[&selectProjectFolderButton] = "Select the Location of the Current Project Folder where to store files.";
        helpTextMap[&storeCompleteConfigButton] = "Store Complete Configuration to files (with backup).";
        helpTextMap[&reloadCompleteConfigButton] = "Reload Complete Configuration from files.";
        helpTextMap[&reloadCompleteConfigBackupButton] = "Reload Complete Configuration from backup files.";
        helpTextMap[&storeSystemConfigButton] = "Store System Configuration to file (with backup).";
        helpTextMap[&reloadSystemConfigButton] = "Reload System Configuration from file.";
        helpTextMap[&reloadSystemConfigBackupButton] = "Reload System Configuration from backup file.";
        helpTextMap[&importSystemConfigButton] = "Import System Configuration from file (with file explorer window).";
        helpTextMap[&exportSystemConfigButton] = "Export System Configuration to file (with file explorer window).";
    }

    void setupMouseListeners()
    {
        // Enable mouse enter/exit events for all components with help text
        for (auto& pair : helpTextMap)
        {
            pair.first->setMouseCursor(juce::MouseCursor::PointingHandCursor);
            // Use true for ComboBoxes to receive events from their internal child components
            bool wantsEventsFromChildren = (dynamic_cast<juce::ComboBox*>(pair.first) != nullptr);
            pair.first->addMouseListener(this, wantsEventsFromChildren);
        }
    }

    void mouseEnter(const juce::MouseEvent& event) override
    {
        if (statusBar == nullptr) return;

        // Walk up parent chain to find a registered component (needed for ComboBox children)
        juce::Component* component = event.eventComponent;
        while (component != nullptr)
        {
            if (helpTextMap.find(component) != helpTextMap.end())
            {
                statusBar->setHelpText(helpTextMap[component]);
                return;
            }
            component = component->getParentComponent();
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (statusBar != nullptr)
            statusBar->clearText();
    }

    //==============================================================================
    // Member variables

    // Column X positions for consistent layout
    static constexpr int col1X = 20;
    static constexpr int col2X = 480;
    static constexpr int col3X = 940;

    WfsParameters& parameters;
    juce::ValueTree configTree;  // Store for safe listener removal in destructor
    StatusBar* statusBar = nullptr;
    std::map<juce::Component*, juce::String> helpTextMap;
    bool processingEnabled = false;
    bool isValidatingFromReturnKey = false;  // Prevents double validation when Enter triggers both ReturnKey and FocusLost
    bool isShowingChannelReductionDialog = false;  // Prevents multiple dialogs from appearing

    // Show Section
    juce::Label showNameLabel;
    juce::TextEditor showNameEditor;
    juce::Label showLocationLabel;
    juce::TextEditor showLocationEditor;

    // I/O Section
    juce::Label inputChannelsLabel;
    juce::TextEditor inputChannelsEditor;
    juce::Label outputChannelsLabel;
    juce::TextEditor outputChannelsEditor;
    juce::Label reverbChannelsLabel;
    juce::TextEditor reverbChannelsEditor;
    juce::TextButton audioPatchingButton;
    juce::Label algorithmLabel;
    juce::ComboBox algorithmSelector;
    juce::TextButton processingButton;

    // Stage Section
    juce::Label stageShapeLabel;
    juce::ComboBox stageShapeSelector;
    juce::Label stageWidthLabel;
    juce::TextEditor stageWidthEditor;
    juce::Label stageWidthUnitLabel;
    juce::Label stageDepthLabel;
    juce::TextEditor stageDepthEditor;
    juce::Label stageDepthUnitLabel;
    juce::Label stageHeightLabel;
    juce::TextEditor stageHeightEditor;
    juce::Label stageHeightUnitLabel;
    juce::Label stageDiameterLabel;
    juce::TextEditor stageDiameterEditor;
    juce::Label stageDiameterUnitLabel;
    juce::Label domeElevationLabel;
    juce::TextEditor domeElevationEditor;
    juce::Label domeElevationUnitLabel;
    juce::Label stageOriginWidthLabel;
    juce::TextEditor stageOriginWidthEditor;
    juce::Label stageOriginWidthUnitLabel;
    juce::Label stageOriginDepthLabel;
    juce::TextEditor stageOriginDepthEditor;
    juce::Label stageOriginDepthUnitLabel;
    juce::Label stageOriginHeightLabel;
    juce::TextEditor stageOriginHeightEditor;
    juce::Label stageOriginHeightUnitLabel;
    OriginFrontButton originFrontButton;
    OriginCenterGroundButton originCenterGroundButton;
    OriginCenterButton originCenterButton;
    juce::Label speedOfSoundLabel;
    juce::TextEditor speedOfSoundEditor;
    juce::Label speedOfSoundUnitLabel;
    juce::Label temperatureLabel;
    juce::TextEditor temperatureEditor;
    juce::Label temperatureUnitLabel;

    // Master Section
    juce::Label masterLevelLabel;
    juce::TextEditor masterLevelEditor;
    juce::Label masterLevelUnitLabel;
    juce::Label systemLatencyLabel;
    juce::TextEditor systemLatencyEditor;
    juce::Label systemLatencyUnitLabel;
    juce::Label haasEffectLabel;
    juce::TextEditor haasEffectEditor;
    juce::Label haasEffectUnitLabel;

    // UI Section
    juce::Label colorSchemeLabel;
    juce::ComboBox colorSchemeSelector;

    // Store/Reload Section
    juce::TextButton selectProjectFolderButton;
    juce::TextButton storeCompleteConfigButton;
    juce::TextButton reloadCompleteConfigButton;
    juce::TextButton reloadCompleteConfigBackupButton;
    juce::TextButton storeSystemConfigButton;
    juce::TextButton reloadSystemConfigButton;
    juce::TextButton reloadSystemConfigBackupButton;
    juce::TextButton importSystemConfigButton;
    juce::TextButton exportSystemConfigButton;
    juce::File projectFolder;

    // Callbacks for notifying MainComponent
    ProcessingCallback onProcessingChanged;
    ChannelCountCallback onChannelCountChanged;
    AudioInterfaceCallback onAudioInterfaceWindowRequested;
    ConfigReloadedCallback onConfigReloaded;

    // Helper to notify MainComponent of any channel count change
    void notifyChannelCountChanged()
    {
        if (onChannelCountChanged)
        {
            int inputs = parameters.getNumInputChannels();
            int outputs = parameters.getNumOutputChannels();
            int reverbs = parameters.getNumReverbChannels();
            onChannelCountChanged(inputs, outputs, reverbs);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SystemConfigTab)
};
