#include "OutputArrayHelperWindow.h"
#include "ColorScheme.h"

//==============================================================================
// Preset configurations
//==============================================================================

const std::array<ArrayPresetConfig, 7> OutputArrayHelperContent::presetConfigs = {{
    // Near Field Array Straight
    {
        ArrayPresetType::NearFieldStraight,
        "Near Field Array Straight",
        true,   // supportsCenterSpacing
        true,   // supportsEndpoints
        false,  // supportsCurve
        false,  // supportsCircle
        false,  // supportsSurround
        true,   // lsAttenEnable
        true,   // frEnable (Floor Reflections)
        -0.4f,  // hfDamping
        2.0f,   // hParallax
        0.5f,   // vParallax
        100,    // distanceAttenPercent
        true,   // hasLowCut
        80,     // lowCutFreq
        false,  // hasHighCut
        300     // highCutFreq (unused)
    },
    // Near Field Array Curved
    {
        ArrayPresetType::NearFieldCurved,
        "Near Field Array Curved",
        false,  // supportsCenterSpacing
        true,   // supportsEndpoints
        true,   // supportsCurve
        false,  // supportsCircle
        false,  // supportsSurround
        true,   // lsAttenEnable
        true,   // frEnable (Floor Reflections)
        -0.4f,  // hfDamping
        2.0f,   // hParallax
        0.5f,   // vParallax
        100,    // distanceAttenPercent
        true,   // hasLowCut
        80,     // lowCutFreq
        false,  // hasHighCut
        300     // highCutFreq (unused)
    },
    // Main Room Array Straight
    {
        ArrayPresetType::MainRoomStraight,
        "Main Room Array Straight",
        true,   // supportsCenterSpacing
        true,   // supportsEndpoints
        false,  // supportsCurve
        false,  // supportsCircle
        false,  // supportsSurround
        false,  // lsAttenEnable
        false,  // frEnable (Floor Reflections)
        -0.2f,  // hfDamping
        10.0f,  // hParallax
        -4.0f,  // vParallax
        100,    // distanceAttenPercent
        false,  // hasLowCut
        80,     // lowCutFreq (unused)
        false,  // hasHighCut
        300     // highCutFreq (unused)
    },
    // Sub Bass
    {
        ArrayPresetType::SubBass,
        "Sub Bass",
        true,   // supportsCenterSpacing
        true,   // supportsEndpoints
        false,  // supportsCurve
        false,  // supportsCircle
        false,  // supportsSurround
        false,  // lsAttenEnable
        false,  // frEnable (Floor Reflections)
        0.0f,   // hfDamping
        0.0f,   // hParallax
        0.0f,   // vParallax
        50,     // distanceAttenPercent (50% for N<=2, adjusted dynamically)
        false,  // hasLowCut
        80,     // lowCutFreq (unused)
        true,   // hasHighCut
        300     // highCutFreq
    },
    // Surround
    {
        ArrayPresetType::Surround,
        "Surround",
        false,  // supportsCenterSpacing
        false,  // supportsEndpoints
        false,  // supportsCurve
        false,  // supportsCircle
        true,   // supportsSurround
        false,  // lsAttenEnable
        false,  // frEnable (Floor Reflections)
        -0.3f,  // hfDamping
        3.0f,   // hParallax
        -2.0f,  // vParallax
        100,    // distanceAttenPercent
        false,  // hasLowCut
        80,     // lowCutFreq (unused)
        false,  // hasHighCut
        300     // highCutFreq (unused)
    },
    // Delay Line
    {
        ArrayPresetType::DelayLine,
        "Delay Line",
        true,   // supportsCenterSpacing
        true,   // supportsEndpoints
        false,  // supportsCurve
        false,  // supportsCircle
        false,  // supportsSurround
        false,  // lsAttenEnable
        false,  // frEnable (Floor Reflections)
        -0.15f, // hfDamping
        3.0f,   // hParallax
        -2.0f,  // vParallax
        100,    // distanceAttenPercent
        false,  // hasLowCut
        80,     // lowCutFreq (unused)
        false,  // hasHighCut
        300     // highCutFreq (unused)
    },
    // Circle
    {
        ArrayPresetType::Circle,
        "Circle",
        false,  // supportsCenterSpacing
        false,  // supportsEndpoints
        false,  // supportsCurve
        true,   // supportsCircle
        false,  // supportsSurround
        false,  // lsAttenEnable
        false,  // frEnable (Floor Reflections)
        -0.3f,  // hfDamping
        0.0f,   // hParallax
        0.0f,   // vParallax
        100,    // distanceAttenPercent
        false,  // hasLowCut
        80,     // lowCutFreq (unused)
        false,  // hasHighCut
        300     // highCutFreq (unused)
    }
}};

//==============================================================================
// ArrayPreviewComponent
//==============================================================================

ArrayPreviewComponent::ArrayPreviewComponent(WfsParameters& params)
    : parameters(params)
{
    setOpaque(true);
}

void ArrayPreviewComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background - use theme color
    g.fillAll(ColorScheme::get().backgroundAlt);

    // Get stage shape and dimensions from parameters
    int stageShape = parameters.getConfigParam("stageShape").isVoid() ? 0 :
                     static_cast<int>(parameters.getConfigParam("stageShape"));
    float stageWidth = parameters.getConfigParam("stageWidth").isVoid() ? 20.0f :
                       static_cast<float>(parameters.getConfigParam("stageWidth"));
    float stageDepth = parameters.getConfigParam("stageDepth").isVoid() ? 15.0f :
                       static_cast<float>(parameters.getConfigParam("stageDepth"));
    float stageDiameter = parameters.getConfigParam("stageDiameter").isVoid() ? 20.0f :
                          static_cast<float>(parameters.getConfigParam("stageDiameter"));
    float originX = parameters.getConfigParam("originWidth").isVoid() ? 0.0f :
                    static_cast<float>(parameters.getConfigParam("originWidth"));
    float originY = parameters.getConfigParam("originDepth").isVoid() ? 0.0f :
                    static_cast<float>(parameters.getConfigParam("originDepth"));

    // Determine stage size based on shape
    // For cylinder/dome (shapes 1 and 2), use diameter for both dimensions
    bool isCircular = (stageShape != 0);
    float stageExtentX = isCircular ? stageDiameter : stageWidth;
    float stageExtentY = isCircular ? stageDiameter : stageDepth;

    // Calculate transform to fit stage in view with padding
    const float padding = 20.0f;
    float viewWidth = bounds.getWidth() - padding * 2;
    float viewHeight = bounds.getHeight() - padding * 2;

    float scaleX = viewWidth / stageExtentX;
    float scaleY = viewHeight / stageExtentY;
    scale = juce::jmin(scaleX, scaleY);

    // Center the stage in the view (origin is at center for center-referenced system)
    float scaledWidth = stageExtentX * scale;
    float scaledHeight = stageExtentY * scale;
    offsetX = padding + (viewWidth - scaledWidth) / 2.0f + (stageExtentX / 2.0f + originX) * scale;
    offsetY = padding + (viewHeight - scaledHeight) / 2.0f + (stageExtentY / 2.0f + originY) * scale;

    // Draw stage bounds
    if (isCircular)
    {
        // Cylinder or Dome - draw circle
        float radius = stageDiameter / 2.0f;
        auto center = stageToScreen(-originX, -originY);
        float radiusPixels = radius * scale;

        g.setColour(ColorScheme::get().surfaceCard);
        g.fillEllipse(center.x - radiusPixels, center.y - radiusPixels,
                      radiusPixels * 2.0f, radiusPixels * 2.0f);
        g.setColour(ColorScheme::get().textPrimary.withAlpha(0.5f));
        g.drawEllipse(center.x - radiusPixels, center.y - radiusPixels,
                      radiusPixels * 2.0f, radiusPixels * 2.0f, 1.0f);
    }
    else
    {
        // Box - draw rectangle
        float halfWidth = stageWidth / 2.0f;
        float halfDepth = stageDepth / 2.0f;
        auto stageTopLeft = stageToScreen(-halfWidth - originX, halfDepth - originY);
        auto stageBottomRight = stageToScreen(halfWidth - originX, -halfDepth - originY);

        juce::Rectangle<float> stageRect(stageTopLeft.x, stageTopLeft.y,
                                          stageBottomRight.x - stageTopLeft.x,
                                          stageBottomRight.y - stageTopLeft.y);

        g.setColour(ColorScheme::get().surfaceCard);
        g.fillRect(stageRect);
        g.setColour(ColorScheme::get().textPrimary.withAlpha(0.5f));
        g.drawRect(stageRect, 1.0f);
    }

    // Draw grid lines (1m spacing)
    g.setColour(ColorScheme::get().chromeDivider);
    float halfExtentX = stageExtentX / 2.0f;
    float halfExtentY = stageExtentY / 2.0f;
    for (float x = -halfExtentX - originX; x <= halfExtentX - originX; x += 1.0f)
    {
        auto top = stageToScreen(x, halfExtentY - originY);
        auto bottom = stageToScreen(x, -halfExtentY - originY);
        g.drawLine(top.x, top.y, bottom.x, bottom.y, 0.5f);
    }
    for (float y = -halfExtentY - originY; y <= halfExtentY - originY; y += 1.0f)
    {
        auto left = stageToScreen(-halfExtentX - originX, y);
        auto right = stageToScreen(halfExtentX - originX, y);
        g.drawLine(left.x, left.y, right.x, right.y, 0.5f);
    }

    // Draw origin marker
    auto origin = stageToScreen(0, 0);
    g.setColour(ColorScheme::get().textPrimary);
    g.drawLine(origin.x - 10, origin.y, origin.x + 10, origin.y, 1.0f);
    g.drawLine(origin.x, origin.y - 10, origin.x, origin.y + 10, 1.0f);
    g.drawEllipse(origin.x - 5, origin.y - 5, 10, 10, 1.0f);

    // Draw calculated speaker positions
    const float speakerRadius = 8.0f;
    const float arrowLength = 15.0f;

    for (size_t i = 0; i < speakerPositions.size(); ++i)
    {
        const auto& pos = speakerPositions[i];
        auto screenPos = stageToScreen(pos.x, pos.y);

        // Draw speaker circle
        g.setColour(ColorScheme::get().accentBlue);
        g.fillEllipse(screenPos.x - speakerRadius, screenPos.y - speakerRadius,
                      speakerRadius * 2, speakerRadius * 2);

        // Draw orientation arrow (matching MapTab convention)
        // 0 degrees = facing back of stage (toward +Y stage, -Y screen = up)
        // 180 degrees = facing audience (toward -Y stage, +Y screen = down)
        float angleRad = juce::degreesToRadians(pos.orientation - 90.0f);
        float arrowDx = std::cos(angleRad) * arrowLength;
        float arrowDy = std::sin(angleRad) * arrowLength;

        g.setColour(ColorScheme::get().textPrimary);
        g.drawArrow(juce::Line<float>(screenPos.x, screenPos.y,
                                       screenPos.x + arrowDx, screenPos.y + arrowDy),
                    2.0f, 6.0f, 6.0f);

        // Draw speaker number
        g.setColour(ColorScheme::get().textPrimary);
        g.setFont(10.0f);
        g.drawText(juce::String(i + 1),
                   juce::Rectangle<float>(screenPos.x - 15, screenPos.y - speakerRadius - 15, 30, 12),
                   juce::Justification::centred);
    }

    // Draw "Audience" label(s) based on stage shape and preset
    g.setColour(ColorScheme::get().textSecondary);
    g.setFont(12.0f);

    // isCircular already defined above
    bool isCirclePreset = (currentPreset == ArrayPresetType::Circle);

    if (isCirclePreset)
    {
        // Circle preset
        if (circleFacingInward)
        {
            // Facing inward: audience in the center of the circle
            auto center = stageToScreen(-originX, -originY);
            g.drawText("Audience",
                       juce::Rectangle<float>(center.x - 40, center.y - 8, 80, 16),
                       juce::Justification::centred);
        }
        else
        {
            // Facing outward: audience at top and bottom
            float radius = circleRadius > 0 ? circleRadius : 5.0f;

            auto topPos = stageToScreen(-originX, -originY + radius + 2.0f);
            auto bottomPos = stageToScreen(-originX, -originY - radius - 2.0f);

            g.drawText("Audience",
                       juce::Rectangle<float>(topPos.x - 40, topPos.y - 16, 80, 16),
                       juce::Justification::centred);
            g.drawText("Audience",
                       juce::Rectangle<float>(bottomPos.x - 40, bottomPos.y, 80, 16),
                       juce::Justification::centred);
        }
    }
    else if (!isCircular)
    {
        // Box mode: audience label in middle of space beneath the stage
        float halfDepth = stageDepth / 2.0f;
        auto stageBottom = stageToScreen(0, -halfDepth - originY);
        float spaceBelow = bounds.getBottom() - stageBottom.y;

        g.drawText("Audience",
                   juce::Rectangle<float>(bounds.getX(), stageBottom.y + spaceBelow / 2.0f - 8,
                                          bounds.getWidth(), 16),
                   juce::Justification::centred);
    }
    // For Cylinder/Dome with non-circle preset: no audience label
}

void ArrayPreviewComponent::resized()
{
    repaint();
}

void ArrayPreviewComponent::setPositions(const std::vector<SpeakerPosition>& positions)
{
    speakerPositions = positions;
    repaint();
}

void ArrayPreviewComponent::clearPositions()
{
    speakerPositions.clear();
    repaint();
}

void ArrayPreviewComponent::setPresetInfo(ArrayPresetType preset, bool circleInward, float radius)
{
    currentPreset = preset;
    circleFacingInward = circleInward;
    circleRadius = radius;
    repaint();
}

juce::Point<float> ArrayPreviewComponent::stageToScreen(float stageX, float stageY) const
{
    // Stage Y+ is toward back of stage, screen Y+ is downward
    // So we invert Y
    float screenX = offsetX + stageX * scale;
    float screenY = offsetY - stageY * scale;
    return { screenX, screenY };
}

//==============================================================================
// OutputArrayHelperContent
//==============================================================================

OutputArrayHelperContent::OutputArrayHelperContent(WfsParameters& params)
    : parameters(params)
{
    setOpaque(true);

    setupPresetSelector();
    setupGeometrySection();
    setupAcousticSection();
    setupTargetSection();
    setupButtons();

    // Create preview
    preview = std::make_unique<ArrayPreviewComponent>(parameters);
    addAndMakeVisible(*preview);

    // Load initial preset defaults
    loadPresetDefaults(ArrayPresetType::NearFieldStraight);
    updateGeometryVisibility();

    // Initial preview calculation (deferred to ensure layout is ready)
    juce::MessageManager::callAsync([this]() { autoCalculatePreview(); });
}

OutputArrayHelperContent::~OutputArrayHelperContent() = default;

void OutputArrayHelperContent::setupPresetSelector()
{
    addAndMakeVisible(presetLabel);
    presetLabel.setText("Preset:", juce::dontSendNotification);

    addAndMakeVisible(presetSelector);
    for (size_t i = 0; i < presetConfigs.size(); ++i)
    {
        presetSelector.addItem(presetConfigs[i].name, static_cast<int>(i) + 1);
    }
    presetSelector.setSelectedId(1, juce::dontSendNotification);
    presetSelector.onChange = [this]() { onPresetChanged(); };
}

void OutputArrayHelperContent::setupGeometrySection()
{
    addAndMakeVisible(geometryGroup);
    geometryGroup.setText("Geometry");
    // Colors handled by WfsLookAndFeel

    // Geometry method radio buttons
    addAndMakeVisible(centerSpacingRadio);
    centerSpacingRadio.setButtonText("Center + Spacing");
    centerSpacingRadio.setRadioGroupId(1);
    centerSpacingRadio.setToggleState(true, juce::dontSendNotification);
    centerSpacingRadio.onClick = [this]() { updateGeometryVisibility(); autoCalculatePreview(); };

    addAndMakeVisible(endpointsRadio);
    endpointsRadio.setButtonText("Endpoints");
    endpointsRadio.setRadioGroupId(1);
    endpointsRadio.onClick = [this]() { updateGeometryVisibility(); autoCalculatePreview(); };

    // Common fields
    auto setupLabel = [this](juce::Label& label, const juce::String& text) {
        addAndMakeVisible(label);
        label.setText(text, juce::dontSendNotification);
    };

    auto setupEditor = [this](juce::TextEditor& editor, const juce::String& defaultText = "") {
        addAndMakeVisible(editor);
        editor.setText(defaultText);
        // Colors handled by WfsLookAndFeel
        // Auto-calculate preview on text change
        editor.onTextChange = [this]() { autoCalculatePreview(); };
    };

    setupLabel(numSpeakersLabel, "N Speakers:");
    setupEditor(numSpeakersEditor, "8");
    numSpeakersEditor.onTextChange = [this]() {
        // For Sub Bass preset, update distance attenuation based on speaker count
        if (currentPreset == ArrayPresetType::SubBass)
        {
            int n = numSpeakersEditor.getText().getIntValue();
            distanceAttenEditor.setText(n <= 2 ? "50" : "100");
        }
        autoCalculatePreview();
    };

    setupLabel(zPositionLabel, "Z Height (m):");
    setupEditor(zPositionEditor, "0");

    setupLabel(orientationLabel, "Orientation (deg):");
    setupEditor(orientationEditor, "0");

    // Center + Spacing fields
    setupLabel(centerXLabel, "Center X (m):");
    setupEditor(centerXEditor, "0");

    setupLabel(centerYLabel, "Center Y (m):");
    setupEditor(centerYEditor, "0");

    setupLabel(spacingLabel, "Spacing (m):");
    setupEditor(spacingEditor, "1");

    // Endpoints fields
    setupLabel(startXLabel, "Start X (m):");
    setupEditor(startXEditor, "-4");

    setupLabel(startYLabel, "Start Y (m):");
    setupEditor(startYEditor, "0");

    setupLabel(endXLabel, "End X (m):");
    setupEditor(endXEditor, "4");

    setupLabel(endYLabel, "End Y (m):");
    setupEditor(endYEditor, "0");

    // Curved array
    setupLabel(sagLabel, "Sag (m):");
    setupEditor(sagEditor, "1");

    // Circle array
    setupLabel(radiusLabel, "Radius (m):");
    setupEditor(radiusEditor, "5");

    setupLabel(startAngleLabel, "Start Angle (deg):");
    setupEditor(startAngleEditor, "0");

    addAndMakeVisible(facingInwardRadio);
    facingInwardRadio.setButtonText("Facing Inward");
    facingInwardRadio.setRadioGroupId(2);
    facingInwardRadio.setToggleState(true, juce::dontSendNotification);
    facingInwardRadio.onClick = [this]() { autoCalculatePreview(); };

    addAndMakeVisible(facingOutwardRadio);
    facingOutwardRadio.setButtonText("Facing Outward");
    facingOutwardRadio.setRadioGroupId(2);
    facingOutwardRadio.onClick = [this]() { autoCalculatePreview(); };

    // Surround
    setupLabel(widthLabel, "Width (m):");
    setupEditor(widthEditor, "8");

    setupLabel(yStartLabel, "Y Start (m):");
    setupEditor(yStartEditor, "2");

    setupLabel(yEndLabel, "Y End (m):");
    setupEditor(yEndEditor, "10");

    // Delay line
    addAndMakeVisible(frontFacingRadio);
    frontFacingRadio.setButtonText("Front Facing");
    frontFacingRadio.setRadioGroupId(3);
    frontFacingRadio.setToggleState(true, juce::dontSendNotification);
    frontFacingRadio.onClick = [this]() { autoCalculatePreview(); };

    addAndMakeVisible(backFacingRadio);
    backFacingRadio.setButtonText("Back Facing");
    backFacingRadio.setRadioGroupId(3);
    backFacingRadio.onClick = [this]() { autoCalculatePreview(); };
}

void OutputArrayHelperContent::setupAcousticSection()
{
    addAndMakeVisible(acousticGroup);
    acousticGroup.setText("Acoustic Defaults");
    // Colors handled by WfsLookAndFeel

    auto setupLabel = [this](juce::Label& label, const juce::String& text) {
        addAndMakeVisible(label);
        label.setText(text, juce::dontSendNotification);
    };

    auto setupEditor = [this](juce::TextEditor& editor, const juce::String& defaultText = "") {
        addAndMakeVisible(editor);
        editor.setText(defaultText);
        // Colors handled by WfsLookAndFeel
    };

    addAndMakeVisible(lsEnableButton);
    lsEnableButton.setButtonText("Live Source");
    lsEnableButton.setToggleState(true, juce::dontSendNotification);

    addAndMakeVisible(frEnableButton);
    frEnableButton.setButtonText("Floor Reflections");
    frEnableButton.setToggleState(true, juce::dontSendNotification);

    setupLabel(hfDampingLabel, "HF Damping (dB/m):");
    setupEditor(hfDampingEditor, "-0.4");

    setupLabel(hParallaxLabel, "H Parallax (m):");
    setupEditor(hParallaxEditor, "2");

    setupLabel(vParallaxLabel, "V Parallax (m):");
    setupEditor(vParallaxEditor, "0.5");

    setupLabel(distanceAttenLabel, "Distance Atten (%):");
    setupEditor(distanceAttenEditor, "100");

    // EQ
    addAndMakeVisible(lowCutEnableButton);
    lowCutEnableButton.setButtonText("");
    lowCutEnableButton.setToggleState(true, juce::dontSendNotification);

    setupLabel(lowCutFreqLabel, "Low Cut (Hz):");
    setupEditor(lowCutFreqEditor, "80");

    addAndMakeVisible(highCutEnableButton);
    highCutEnableButton.setButtonText("");
    highCutEnableButton.setToggleState(false, juce::dontSendNotification);

    setupLabel(highCutFreqLabel, "High Cut (Hz):");
    setupEditor(highCutFreqEditor, "300");
}

void OutputArrayHelperContent::setupTargetSection()
{
    addAndMakeVisible(targetGroup);
    targetGroup.setText("Target");
    // Colors handled by WfsLookAndFeel

    addAndMakeVisible(arrayLabel);
    arrayLabel.setText("Array:", juce::dontSendNotification);

    addAndMakeVisible(arraySelector);
    for (int i = 1; i <= 10; ++i)
        arraySelector.addItem("Array " + juce::String(i), i);
    arraySelector.setSelectedId(1, juce::dontSendNotification);

    addAndMakeVisible(startOutputLabel);
    startOutputLabel.setText("Starting Output:", juce::dontSendNotification);

    addAndMakeVisible(startOutputSelector);
    int numOutputs = parameters.getNumOutputChannels();
    if (numOutputs <= 0) numOutputs = 64;
    for (int i = 1; i <= numOutputs; ++i)
        startOutputSelector.addItem(juce::String(i), i);
    startOutputSelector.setSelectedId(1, juce::dontSendNotification);
}

void OutputArrayHelperContent::setupButtons()
{
    addAndMakeVisible(applyButton);
    applyButton.setButtonText("Apply");
    applyButton.setColour(juce::TextButton::buttonColourId, ColorScheme::get().accentGreen);
    applyButton.onClick = [this]() { applyToOutputs(); };

    addAndMakeVisible(closeButton);
    closeButton.setButtonText("Close");
    closeButton.setColour(juce::TextButton::buttonColourId, ColorScheme::get().accentRed);
    closeButton.onClick = [this]() {
        if (auto* window = findParentComponentOfClass<OutputArrayHelperWindow>())
            window->setVisible(false);
    };

    addAndMakeVisible(statusLabel);
    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, ColorScheme::get().textSecondary);
}

void OutputArrayHelperContent::paint(juce::Graphics& g)
{
    g.fillAll(ColorScheme::get().background);
}

void OutputArrayHelperContent::resized()
{
    auto bounds = getLocalBounds();
    const int padding = 10;
    const int rowHeight = 28;

    // Footer with buttons
    auto footer = bounds.removeFromBottom(50).reduced(padding, 10);
    auto buttonWidth = 100;
    auto buttonSpacing = 10;

    closeButton.setBounds(footer.removeFromRight(buttonWidth));
    footer.removeFromRight(buttonSpacing);
    applyButton.setBounds(footer.removeFromRight(buttonWidth));

    statusLabel.setBounds(footer.reduced(5, 0));

    // Main content area
    auto contentArea = bounds.reduced(padding);

    // Split: left panel (controls) and right panel (preview)
    auto leftPanel = contentArea.removeFromLeft(contentArea.getWidth() / 2 - padding / 2);
    contentArea.removeFromLeft(padding);
    auto rightPanel = contentArea;

    // Preview takes the right panel
    preview->setBounds(rightPanel);

    // Left panel layout
    auto area = leftPanel;

    // Preset selector
    auto presetRow = area.removeFromTop(rowHeight);
    presetLabel.setBounds(presetRow.removeFromLeft(80));
    presetSelector.setBounds(presetRow);
    area.removeFromTop(padding);

    // Geometry section
    layoutGeometrySection(area);
    area.removeFromTop(padding);

    // Acoustic section
    layoutAcousticSection(area);
    area.removeFromTop(padding);

    // Target section
    layoutTargetSection(area);
}

void OutputArrayHelperContent::layoutGeometrySection(juce::Rectangle<int>& area)
{
    const int rowHeight = 26;
    const int editorWidth = 60;
    const int colSpacing = 10;
    const int checkboxWidth = 25;
    const int labelWidth = 110;

    auto& config = presetConfigs[static_cast<int>(currentPreset)];

    // Calculate height needed
    int height = 30;  // Header
    height += rowHeight;  // Radio buttons
    height += rowHeight;  // N Speakers + Z Height
    height += rowHeight;  // Orientation

    if (config.supportsCenterSpacing || config.supportsEndpoints)
    {
        if (centerSpacingRadio.getToggleState() && config.supportsCenterSpacing)
            height += rowHeight * 2;  // Center X/Y, Spacing
        else if (config.supportsEndpoints)
            height += rowHeight * 2;  // Start/End points
    }

    if (config.supportsCurve)
        height += rowHeight;
    if (config.supportsCircle)
        height += rowHeight * 2;
    if (config.supportsSurround)
        height += rowHeight * 2;
    if (currentPreset == ArrayPresetType::DelayLine)
        height += rowHeight;

    height += 15;  // Padding

    auto section = area.removeFromTop(height);
    geometryGroup.setBounds(section);

    auto content = section.reduced(10, 20);
    content.removeFromTop(10);

    // Split into two main columns
    const int columnWidth = content.getWidth() / 2 - colSpacing / 2;

    // Radio buttons row
    if (config.supportsCenterSpacing && config.supportsEndpoints)
    {
        auto radioRow = content.removeFromTop(rowHeight);
        auto leftCol = radioRow.removeFromLeft(columnWidth);
        radioRow.removeFromLeft(colSpacing);
        auto rightCol = radioRow;
        centerSpacingRadio.setBounds(leftCol.removeFromLeft(checkboxWidth + labelWidth + editorWidth));
        endpointsRadio.setBounds(rightCol.removeFromLeft(checkboxWidth + labelWidth + editorWidth));
        centerSpacingRadio.setVisible(true);
        endpointsRadio.setVisible(true);
    }
    else
    {
        centerSpacingRadio.setVisible(false);
        endpointsRadio.setVisible(false);
    }

    // N Speakers + Z Height row
    auto row = content.removeFromTop(rowHeight);
    auto leftCol = row.removeFromLeft(columnWidth);
    row.removeFromLeft(colSpacing);
    auto rightCol = row;
    leftCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
    numSpeakersLabel.setBounds(leftCol.removeFromLeft(labelWidth));
    numSpeakersEditor.setBounds(leftCol.removeFromLeft(editorWidth));
    rightCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
    zPositionLabel.setBounds(rightCol.removeFromLeft(labelWidth));
    zPositionEditor.setBounds(rightCol.removeFromLeft(editorWidth));

    // Orientation row (for straight arrays)
    if (!config.supportsCircle && !config.supportsSurround)
    {
        row = content.removeFromTop(rowHeight);
        leftCol = row.removeFromLeft(columnWidth);
        leftCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
        orientationLabel.setBounds(leftCol.removeFromLeft(labelWidth));
        orientationEditor.setBounds(leftCol.removeFromLeft(editorWidth));
        orientationLabel.setVisible(true);
        orientationEditor.setVisible(true);
    }
    else
    {
        orientationLabel.setVisible(false);
        orientationEditor.setVisible(false);
    }

    // Center + Spacing fields
    bool showCenterSpacing = config.supportsCenterSpacing &&
                             (centerSpacingRadio.getToggleState() || !config.supportsEndpoints);
    centerXLabel.setVisible(showCenterSpacing);
    centerXEditor.setVisible(showCenterSpacing);
    centerYLabel.setVisible(showCenterSpacing);
    centerYEditor.setVisible(showCenterSpacing);
    spacingLabel.setVisible(showCenterSpacing);
    spacingEditor.setVisible(showCenterSpacing);

    if (showCenterSpacing)
    {
        row = content.removeFromTop(rowHeight);
        leftCol = row.removeFromLeft(columnWidth);
        row.removeFromLeft(colSpacing);
        rightCol = row;
        leftCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
        centerXLabel.setBounds(leftCol.removeFromLeft(labelWidth));
        centerXEditor.setBounds(leftCol.removeFromLeft(editorWidth));
        rightCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
        centerYLabel.setBounds(rightCol.removeFromLeft(labelWidth));
        centerYEditor.setBounds(rightCol.removeFromLeft(editorWidth));

        row = content.removeFromTop(rowHeight);
        leftCol = row.removeFromLeft(columnWidth);
        leftCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
        spacingLabel.setBounds(leftCol.removeFromLeft(labelWidth));
        spacingEditor.setBounds(leftCol.removeFromLeft(editorWidth));
    }

    // Endpoints fields
    bool showEndpoints = config.supportsEndpoints &&
                         (endpointsRadio.getToggleState() || !config.supportsCenterSpacing);
    startXLabel.setVisible(showEndpoints);
    startXEditor.setVisible(showEndpoints);
    startYLabel.setVisible(showEndpoints);
    startYEditor.setVisible(showEndpoints);
    endXLabel.setVisible(showEndpoints);
    endXEditor.setVisible(showEndpoints);
    endYLabel.setVisible(showEndpoints);
    endYEditor.setVisible(showEndpoints);

    if (showEndpoints)
    {
        row = content.removeFromTop(rowHeight);
        leftCol = row.removeFromLeft(columnWidth);
        row.removeFromLeft(colSpacing);
        rightCol = row;
        leftCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
        startXLabel.setBounds(leftCol.removeFromLeft(labelWidth));
        startXEditor.setBounds(leftCol.removeFromLeft(editorWidth));
        rightCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
        startYLabel.setBounds(rightCol.removeFromLeft(labelWidth));
        startYEditor.setBounds(rightCol.removeFromLeft(editorWidth));

        row = content.removeFromTop(rowHeight);
        leftCol = row.removeFromLeft(columnWidth);
        row.removeFromLeft(colSpacing);
        rightCol = row;
        leftCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
        endXLabel.setBounds(leftCol.removeFromLeft(labelWidth));
        endXEditor.setBounds(leftCol.removeFromLeft(editorWidth));
        rightCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
        endYLabel.setBounds(rightCol.removeFromLeft(labelWidth));
        endYEditor.setBounds(rightCol.removeFromLeft(editorWidth));
    }

    // Curved array (sag)
    sagLabel.setVisible(config.supportsCurve);
    sagEditor.setVisible(config.supportsCurve);
    if (config.supportsCurve)
    {
        row = content.removeFromTop(rowHeight);
        leftCol = row.removeFromLeft(columnWidth);
        leftCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
        sagLabel.setBounds(leftCol.removeFromLeft(labelWidth));
        sagEditor.setBounds(leftCol.removeFromLeft(editorWidth));
    }

    // Circle array
    radiusLabel.setVisible(config.supportsCircle);
    radiusEditor.setVisible(config.supportsCircle);
    startAngleLabel.setVisible(config.supportsCircle);
    startAngleEditor.setVisible(config.supportsCircle);
    facingInwardRadio.setVisible(config.supportsCircle);
    facingOutwardRadio.setVisible(config.supportsCircle);

    if (config.supportsCircle)
    {
        row = content.removeFromTop(rowHeight);
        leftCol = row.removeFromLeft(columnWidth);
        row.removeFromLeft(colSpacing);
        rightCol = row;
        leftCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
        centerXLabel.setBounds(leftCol.removeFromLeft(labelWidth));
        centerXEditor.setBounds(leftCol.removeFromLeft(editorWidth));
        rightCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
        centerYLabel.setBounds(rightCol.removeFromLeft(labelWidth));
        centerYEditor.setBounds(rightCol.removeFromLeft(editorWidth));
        centerXLabel.setVisible(true);
        centerXEditor.setVisible(true);
        centerYLabel.setVisible(true);
        centerYEditor.setVisible(true);

        row = content.removeFromTop(rowHeight);
        leftCol = row.removeFromLeft(columnWidth);
        row.removeFromLeft(colSpacing);
        rightCol = row;
        leftCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
        radiusLabel.setBounds(leftCol.removeFromLeft(labelWidth));
        radiusEditor.setBounds(leftCol.removeFromLeft(editorWidth));
        rightCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
        startAngleLabel.setBounds(rightCol.removeFromLeft(labelWidth));
        startAngleEditor.setBounds(rightCol.removeFromLeft(editorWidth));

        row = content.removeFromTop(rowHeight);
        leftCol = row.removeFromLeft(columnWidth);
        row.removeFromLeft(colSpacing);
        rightCol = row;
        facingInwardRadio.setBounds(leftCol.removeFromLeft(checkboxWidth + labelWidth + editorWidth));
        facingOutwardRadio.setBounds(rightCol.removeFromLeft(checkboxWidth + labelWidth + editorWidth));
    }

    // Surround
    widthLabel.setVisible(config.supportsSurround);
    widthEditor.setVisible(config.supportsSurround);
    yStartLabel.setVisible(config.supportsSurround);
    yStartEditor.setVisible(config.supportsSurround);
    yEndLabel.setVisible(config.supportsSurround);
    yEndEditor.setVisible(config.supportsSurround);

    if (config.supportsSurround)
    {
        row = content.removeFromTop(rowHeight);
        leftCol = row.removeFromLeft(columnWidth);
        row.removeFromLeft(colSpacing);
        rightCol = row;
        leftCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
        centerXLabel.setBounds(leftCol.removeFromLeft(labelWidth));
        centerXEditor.setBounds(leftCol.removeFromLeft(editorWidth));
        rightCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
        widthLabel.setBounds(rightCol.removeFromLeft(labelWidth));
        widthEditor.setBounds(rightCol.removeFromLeft(editorWidth));
        centerXLabel.setVisible(true);
        centerXEditor.setVisible(true);

        row = content.removeFromTop(rowHeight);
        leftCol = row.removeFromLeft(columnWidth);
        row.removeFromLeft(colSpacing);
        rightCol = row;
        leftCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
        yStartLabel.setBounds(leftCol.removeFromLeft(labelWidth));
        yStartEditor.setBounds(leftCol.removeFromLeft(editorWidth));
        rightCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
        yEndLabel.setBounds(rightCol.removeFromLeft(labelWidth));
        yEndEditor.setBounds(rightCol.removeFromLeft(editorWidth));

        // Note: For surround, numSpeakers is actually number of pairs
        numSpeakersLabel.setText("N Pairs:", juce::dontSendNotification);
    }
    else
    {
        numSpeakersLabel.setText("N Speakers:", juce::dontSendNotification);
    }

    // Delay line front/back
    frontFacingRadio.setVisible(currentPreset == ArrayPresetType::DelayLine);
    backFacingRadio.setVisible(currentPreset == ArrayPresetType::DelayLine);

    if (currentPreset == ArrayPresetType::DelayLine)
    {
        row = content.removeFromTop(rowHeight);
        leftCol = row.removeFromLeft(columnWidth);
        row.removeFromLeft(colSpacing);
        rightCol = row;
        frontFacingRadio.setBounds(leftCol.removeFromLeft(checkboxWidth + labelWidth + editorWidth));
        backFacingRadio.setBounds(rightCol.removeFromLeft(checkboxWidth + labelWidth + editorWidth));
    }
}

void OutputArrayHelperContent::layoutAcousticSection(juce::Rectangle<int>& area)
{
    const int rowHeight = 26;
    const int editorWidth = 60;
    const int colSpacing = 10;
    const int checkboxWidth = 25;
    const int labelWidth = 110;

    int height = 30 + rowHeight * 4 + 15;
    auto section = area.removeFromTop(height);
    acousticGroup.setBounds(section);

    auto content = section.reduced(10, 20);
    content.removeFromTop(10);

    // Split into two main columns
    const int columnWidth = content.getWidth() / 2 - colSpacing / 2;

    // Row 1: Live Source | Floor Reflections
    auto row = content.removeFromTop(rowHeight);
    auto leftCol = row.removeFromLeft(columnWidth);
    row.removeFromLeft(colSpacing);
    auto rightCol = row;
    lsEnableButton.setBounds(leftCol.removeFromLeft(checkboxWidth + labelWidth + editorWidth));
    frEnableButton.setBounds(rightCol.removeFromLeft(checkboxWidth + labelWidth + editorWidth));

    // Row 2: HF Damping | Distance Atten (no checkbox, indent to align)
    row = content.removeFromTop(rowHeight);
    leftCol = row.removeFromLeft(columnWidth);
    row.removeFromLeft(colSpacing);
    rightCol = row;
    leftCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
    hfDampingLabel.setBounds(leftCol.removeFromLeft(labelWidth));
    hfDampingEditor.setBounds(leftCol.removeFromLeft(editorWidth));
    rightCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
    distanceAttenLabel.setBounds(rightCol.removeFromLeft(labelWidth));
    distanceAttenEditor.setBounds(rightCol.removeFromLeft(editorWidth));

    // Row 3: H Parallax | V Parallax (no checkbox, indent to align)
    row = content.removeFromTop(rowHeight);
    leftCol = row.removeFromLeft(columnWidth);
    row.removeFromLeft(colSpacing);
    rightCol = row;
    leftCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
    hParallaxLabel.setBounds(leftCol.removeFromLeft(labelWidth));
    hParallaxEditor.setBounds(leftCol.removeFromLeft(editorWidth));
    rightCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
    vParallaxLabel.setBounds(rightCol.removeFromLeft(labelWidth));
    vParallaxEditor.setBounds(rightCol.removeFromLeft(editorWidth));

    // Row 4: Low Cut | High Cut (checkbox + label + editor)
    row = content.removeFromTop(rowHeight);
    leftCol = row.removeFromLeft(columnWidth);
    row.removeFromLeft(colSpacing);
    rightCol = row;
    lowCutEnableButton.setBounds(leftCol.removeFromLeft(checkboxWidth));
    lowCutFreqLabel.setBounds(leftCol.removeFromLeft(labelWidth));
    lowCutFreqEditor.setBounds(leftCol.removeFromLeft(editorWidth));
    highCutEnableButton.setBounds(rightCol.removeFromLeft(checkboxWidth));
    highCutFreqLabel.setBounds(rightCol.removeFromLeft(labelWidth));
    highCutFreqEditor.setBounds(rightCol.removeFromLeft(editorWidth));
}

void OutputArrayHelperContent::layoutTargetSection(juce::Rectangle<int>& area)
{
    const int rowHeight = 26;
    const int colSpacing = 10;
    const int checkboxWidth = 25;
    const int labelWidth = 110;
    const int selectorWidth = 100;

    int height = 30 + rowHeight + 15;
    auto section = area.removeFromTop(height);
    targetGroup.setBounds(section);

    auto content = section.reduced(10, 20);
    content.removeFromTop(10);

    // Split into two main columns
    const int columnWidth = content.getWidth() / 2 - colSpacing / 2;

    auto row = content.removeFromTop(rowHeight);
    auto leftCol = row.removeFromLeft(columnWidth);
    row.removeFromLeft(colSpacing);
    auto rightCol = row;
    leftCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
    arrayLabel.setBounds(leftCol.removeFromLeft(labelWidth));
    arraySelector.setBounds(leftCol.removeFromLeft(selectorWidth));
    rightCol.removeFromLeft(checkboxWidth);  // Skip checkbox space
    startOutputLabel.setBounds(rightCol.removeFromLeft(labelWidth));
    startOutputSelector.setBounds(rightCol.removeFromLeft(selectorWidth));
}

void OutputArrayHelperContent::onPresetChanged()
{
    int selectedId = presetSelector.getSelectedId();
    if (selectedId < 1 || selectedId > static_cast<int>(presetConfigs.size()))
        return;

    currentPreset = static_cast<ArrayPresetType>(selectedId - 1);
    loadPresetDefaults(currentPreset);
    updateGeometryVisibility();

    // Update preview preset info before calculating
    float radius = (currentPreset == ArrayPresetType::Circle) ? radiusEditor.getText().getFloatValue() : 5.0f;
    preview->setPresetInfo(currentPreset, facingInwardRadio.getToggleState(), radius);

    autoCalculatePreview();  // Auto-calculate with new preset defaults
}

void OutputArrayHelperContent::updateGeometryVisibility()
{
    resized();
    repaint();
}

void OutputArrayHelperContent::loadPresetDefaults(ArrayPresetType preset)
{
    auto& config = presetConfigs[static_cast<int>(preset)];

    // Acoustic defaults
    lsEnableButton.setToggleState(config.lsAttenEnable, juce::dontSendNotification);
    frEnableButton.setToggleState(config.frEnable, juce::dontSendNotification);
    hfDampingEditor.setText(juce::String(config.hfDamping, 2));
    hParallaxEditor.setText(juce::String(config.hParallax, 1));
    vParallaxEditor.setText(juce::String(config.vParallax, 1));
    distanceAttenEditor.setText(juce::String(config.distanceAttenPercent));

    lowCutEnableButton.setToggleState(config.hasLowCut, juce::dontSendNotification);
    lowCutFreqEditor.setText(juce::String(config.lowCutFreq));
    highCutEnableButton.setToggleState(config.hasHighCut, juce::dontSendNotification);
    highCutFreqEditor.setText(juce::String(config.highCutFreq));

    // Geometry defaults per preset
    switch (preset)
    {
        case ArrayPresetType::NearFieldStraight:
            numSpeakersEditor.setText("8");
            centerXEditor.setText("0");
            centerYEditor.setText("-0.5");
            spacingEditor.setText("1");
            startXEditor.setText("-4");
            startYEditor.setText("-0.5");
            endXEditor.setText("4");
            endYEditor.setText("-0.5");
            orientationEditor.setText("0");
            break;

        case ArrayPresetType::NearFieldCurved:
            numSpeakersEditor.setText("8");
            startXEditor.setText("-4");
            startYEditor.setText("0");
            endXEditor.setText("4");
            endYEditor.setText("0");
            sagEditor.setText("-1");  // Negative sag = toward audience
            break;

        case ArrayPresetType::MainRoomStraight:
            numSpeakersEditor.setText("8");
            centerXEditor.setText("0");
            centerYEditor.setText("-0.5");
            spacingEditor.setText("2");
            startXEditor.setText("-8");
            startYEditor.setText("-0.5");
            endXEditor.setText("8");
            endYEditor.setText("-0.5");
            orientationEditor.setText("0");
            break;

        case ArrayPresetType::SubBass:
            numSpeakersEditor.setText("2");
            centerXEditor.setText("0");
            centerYEditor.setText("0");
            spacingEditor.setText("16");
            startXEditor.setText("-8");
            startYEditor.setText("0");
            endXEditor.setText("8");
            endYEditor.setText("0");
            orientationEditor.setText("0");
            // Select endpoints method by default for sub bass
            endpointsRadio.setToggleState(true, juce::dontSendNotification);
            break;

        case ArrayPresetType::Surround:
            numSpeakersEditor.setText("2");  // 2 pairs = 4 speakers
            centerXEditor.setText("0");
            widthEditor.setText("8");
            yStartEditor.setText("-4");
            yEndEditor.setText("0");  // 4m Y spacing between pairs
            break;

        case ArrayPresetType::DelayLine:
            numSpeakersEditor.setText("4");
            centerXEditor.setText("0");
            centerYEditor.setText("-12");  // Above audience
            spacingEditor.setText("4");
            startXEditor.setText("-6");
            startYEditor.setText("-12");
            endXEditor.setText("6");
            endYEditor.setText("-12");
            break;

        case ArrayPresetType::Circle:
            numSpeakersEditor.setText("12");
            centerXEditor.setText("0");
            centerYEditor.setText("0");  // Center at origin by default
            radiusEditor.setText("5");
            startAngleEditor.setText("0");
            facingInwardRadio.setToggleState(true, juce::dontSendNotification);
            break;
    }
}

void OutputArrayHelperContent::autoCalculatePreview()
{
    // Silently calculate and update preview without showing status
    auto& config = presetConfigs[static_cast<int>(currentPreset)];

    int numSpeakers = numSpeakersEditor.getText().getIntValue();
    if (numSpeakers <= 0)
    {
        preview->clearPositions();
        return;
    }

    float z = zPositionEditor.getText().getFloatValue();
    float orientation = orientationEditor.getText().getFloatValue();

    std::vector<SpeakerPosition> positions;

    switch (currentPreset)
    {
        case ArrayPresetType::NearFieldStraight:
        case ArrayPresetType::MainRoomStraight:
        case ArrayPresetType::SubBass:
        {
            if (centerSpacingRadio.getToggleState() && config.supportsCenterSpacing)
            {
                float cx = centerXEditor.getText().getFloatValue();
                float cy = centerYEditor.getText().getFloatValue();
                float spacing = spacingEditor.getText().getFloatValue();
                positions = ArrayGeometry::calculateStraightFromCenter(
                    numSpeakers, cx, cy, z, spacing, orientation);
            }
            else
            {
                float x1 = startXEditor.getText().getFloatValue();
                float y1 = startYEditor.getText().getFloatValue();
                float x2 = endXEditor.getText().getFloatValue();
                float y2 = endYEditor.getText().getFloatValue();
                positions = ArrayGeometry::calculateStraightFromEndpoints(
                    numSpeakers, x1, y1, x2, y2, z, orientation);
            }
            break;
        }

        case ArrayPresetType::NearFieldCurved:
        {
            float x1 = startXEditor.getText().getFloatValue();
            float y1 = startYEditor.getText().getFloatValue();
            float x2 = endXEditor.getText().getFloatValue();
            float y2 = endYEditor.getText().getFloatValue();
            float sag = sagEditor.getText().getFloatValue();
            positions = ArrayGeometry::calculateCurvedArray(
                numSpeakers, x1, y1, x2, y2, sag, z);
            break;
        }

        case ArrayPresetType::DelayLine:
        {
            float delayOrientation = frontFacingRadio.getToggleState() ? 0.0f : 180.0f;
            if (centerSpacingRadio.getToggleState() && config.supportsCenterSpacing)
            {
                float cx = centerXEditor.getText().getFloatValue();
                float cy = centerYEditor.getText().getFloatValue();
                float spacing = spacingEditor.getText().getFloatValue();
                positions = ArrayGeometry::calculateStraightFromCenter(
                    numSpeakers, cx, cy, z, spacing, delayOrientation);
            }
            else
            {
                float x1 = startXEditor.getText().getFloatValue();
                float y1 = startYEditor.getText().getFloatValue();
                float x2 = endXEditor.getText().getFloatValue();
                float y2 = endYEditor.getText().getFloatValue();
                positions = ArrayGeometry::calculateStraightFromEndpoints(
                    numSpeakers, x1, y1, x2, y2, z, delayOrientation);
            }
            break;
        }

        case ArrayPresetType::Circle:
        {
            float cx = centerXEditor.getText().getFloatValue();
            float cy = centerYEditor.getText().getFloatValue();
            float radius = radiusEditor.getText().getFloatValue();
            float startAngle = startAngleEditor.getText().getFloatValue();
            bool inward = facingInwardRadio.getToggleState();
            positions = ArrayGeometry::calculateCircleArray(
                numSpeakers, cx, cy, radius, startAngle, z, inward);
            break;
        }

        case ArrayPresetType::Surround:
        {
            int numPairs = numSpeakers;
            float cx = centerXEditor.getText().getFloatValue();
            float width = widthEditor.getText().getFloatValue();
            float yStart = yStartEditor.getText().getFloatValue();
            float yEnd = yEndEditor.getText().getFloatValue();
            positions = ArrayGeometry::calculateSurroundPairs(
                numPairs, cx, width, yStart, yEnd, z);
            break;
        }
    }

    // Update preview with positions and preset info
    float radius = (currentPreset == ArrayPresetType::Circle) ? radiusEditor.getText().getFloatValue() : 5.0f;
    preview->setPresetInfo(currentPreset, facingInwardRadio.getToggleState(), radius);
    preview->setPositions(positions);
}

void OutputArrayHelperContent::calculatePositions()
{
    auto& config = presetConfigs[static_cast<int>(currentPreset)];

    int numSpeakers = numSpeakersEditor.getText().getIntValue();
    float z = zPositionEditor.getText().getFloatValue();
    float orientation = orientationEditor.getText().getFloatValue();

    if (numSpeakers <= 0)
    {
        showError("Number of speakers must be greater than 0");
        return;
    }

    calculatedPositions.clear();

    switch (currentPreset)
    {
        case ArrayPresetType::NearFieldStraight:
        case ArrayPresetType::MainRoomStraight:
        case ArrayPresetType::SubBass:
        {
            if (centerSpacingRadio.getToggleState() && config.supportsCenterSpacing)
            {
                float cx = centerXEditor.getText().getFloatValue();
                float cy = centerYEditor.getText().getFloatValue();
                float spacing = spacingEditor.getText().getFloatValue();
                calculatedPositions = ArrayGeometry::calculateStraightFromCenter(
                    numSpeakers, cx, cy, z, spacing, orientation);
            }
            else
            {
                float x1 = startXEditor.getText().getFloatValue();
                float y1 = startYEditor.getText().getFloatValue();
                float x2 = endXEditor.getText().getFloatValue();
                float y2 = endYEditor.getText().getFloatValue();
                calculatedPositions = ArrayGeometry::calculateStraightFromEndpoints(
                    numSpeakers, x1, y1, x2, y2, z, orientation);
            }
            break;
        }

        case ArrayPresetType::NearFieldCurved:
        {
            float x1 = startXEditor.getText().getFloatValue();
            float y1 = startYEditor.getText().getFloatValue();
            float x2 = endXEditor.getText().getFloatValue();
            float y2 = endYEditor.getText().getFloatValue();
            float sag = sagEditor.getText().getFloatValue();
            calculatedPositions = ArrayGeometry::calculateCurvedArray(
                numSpeakers, x1, y1, x2, y2, sag, z);
            break;
        }

        case ArrayPresetType::DelayLine:
        {
            float delayOrientation = frontFacingRadio.getToggleState() ? 0.0f : 180.0f;
            if (centerSpacingRadio.getToggleState() && config.supportsCenterSpacing)
            {
                float cx = centerXEditor.getText().getFloatValue();
                float cy = centerYEditor.getText().getFloatValue();
                float spacing = spacingEditor.getText().getFloatValue();
                calculatedPositions = ArrayGeometry::calculateStraightFromCenter(
                    numSpeakers, cx, cy, z, spacing, delayOrientation);
            }
            else
            {
                float x1 = startXEditor.getText().getFloatValue();
                float y1 = startYEditor.getText().getFloatValue();
                float x2 = endXEditor.getText().getFloatValue();
                float y2 = endYEditor.getText().getFloatValue();
                calculatedPositions = ArrayGeometry::calculateStraightFromEndpoints(
                    numSpeakers, x1, y1, x2, y2, z, delayOrientation);
            }
            break;
        }

        case ArrayPresetType::Circle:
        {
            float cx = centerXEditor.getText().getFloatValue();
            float cy = centerYEditor.getText().getFloatValue();
            float radius = radiusEditor.getText().getFloatValue();
            float startAngle = startAngleEditor.getText().getFloatValue();
            bool inward = facingInwardRadio.getToggleState();
            calculatedPositions = ArrayGeometry::calculateCircleArray(
                numSpeakers, cx, cy, radius, startAngle, z, inward);
            break;
        }

        case ArrayPresetType::Surround:
        {
            // numSpeakers is actually number of pairs for surround
            int numPairs = numSpeakers;
            float cx = centerXEditor.getText().getFloatValue();
            float width = widthEditor.getText().getFloatValue();
            float yStart = yStartEditor.getText().getFloatValue();
            float yEnd = yEndEditor.getText().getFloatValue();
            calculatedPositions = ArrayGeometry::calculateSurroundPairs(
                numPairs, cx, width, yStart, yEnd, z);
            break;
        }
    }

    // Update preview with positions and preset info
    float radius = (currentPreset == ArrayPresetType::Circle) ? radiusEditor.getText().getFloatValue() : 5.0f;
    preview->setPresetInfo(currentPreset, facingInwardRadio.getToggleState(), radius);
    preview->setPositions(calculatedPositions);
    showStatus("Calculated " + juce::String(calculatedPositions.size()) + " positions");
}

void OutputArrayHelperContent::applyToOutputs()
{
    // Calculate positions before applying (auto-calculates from current UI values)
    calculatePositions();

    if (calculatedPositions.empty())
    {
        showError("No positions to apply. Check geometry parameters.");
        return;
    }

    auto& vts = parameters.getValueTreeState();
    int arrayId = arraySelector.getSelectedId();
    int startOutput = startOutputSelector.getSelectedId() - 1;  // 0-based
    int N = static_cast<int>(calculatedPositions.size());
    int numOutputs = parameters.getNumOutputChannels();
    if (numOutputs <= 0) numOutputs = 64;

    // Validate
    if (startOutput + N > numOutputs)
    {
        showError("Not enough output channels! Need " + juce::String(N) +
                  " starting from " + juce::String(startOutput + 1));
        return;
    }

    // Get acoustic settings from UI
    bool lsEnabled = lsEnableButton.getToggleState();
    bool frEnabled = frEnableButton.getToggleState();
    float hfDamping = hfDampingEditor.getText().getFloatValue();
    float hParallax = hParallaxEditor.getText().getFloatValue();
    float vParallax = vParallaxEditor.getText().getFloatValue();
    int distAtten = distanceAttenEditor.getText().getIntValue();

    // Sub bass special rule: 50% if N <= 2
    if (currentPreset == ArrayPresetType::SubBass && N <= 2)
        distAtten = 50;

    bool enableLowCut = lowCutEnableButton.getToggleState();
    int lowCutFreq = lowCutFreqEditor.getText().getIntValue();
    bool enableHighCut = highCutEnableButton.getToggleState();
    int highCutFreq = highCutFreqEditor.getText().getIntValue();

    // Begin undo transaction
    if (auto* undoManager = parameters.getUndoManager())
        undoManager->beginNewTransaction("Array Position Helper");

    for (int i = 0; i < N; ++i)
    {
        int idx = startOutput + i;
        const auto& pos = calculatedPositions[i];

        // Use ValueTreeState directly for all parameters
        auto posSection = vts.getOutputPositionSection(idx);
        auto optSection = vts.getOutputOptionsSection(idx);
        auto chanSection = vts.getOutputChannelSection(idx);

        // Position (in Position section)
        posSection.setProperty(WFSParameterIDs::outputPositionX, pos.x, nullptr);
        posSection.setProperty(WFSParameterIDs::outputPositionY, pos.y, nullptr);
        posSection.setProperty(WFSParameterIDs::outputPositionZ, pos.z, nullptr);
        posSection.setProperty(WFSParameterIDs::outputOrientation, static_cast<int>(pos.orientation), nullptr);
        posSection.setProperty(WFSParameterIDs::outputHFdamping, hfDamping, nullptr);

        // Array assignment (in Channel section)
        chanSection.setProperty(WFSParameterIDs::outputArray, arrayId, nullptr);

        // Acoustic defaults (in Options section)
        optSection.setProperty(WFSParameterIDs::outputLSattenEnable, lsEnabled ? 1 : 0, nullptr);
        optSection.setProperty(WFSParameterIDs::outputFRenable, frEnabled ? 1 : 0, nullptr);
        optSection.setProperty(WFSParameterIDs::outputHparallax, hParallax, nullptr);
        optSection.setProperty(WFSParameterIDs::outputVparallax, vParallax, nullptr);
        optSection.setProperty(WFSParameterIDs::outputDistanceAttenPercent, distAtten, nullptr);

        // EQ bands if configured (Low Cut = band 0, High Cut = band 5)
        if (enableLowCut)
        {
            auto eqBand0 = vts.getOutputEQBand(idx, 0);
            eqBand0.setProperty(WFSParameterIDs::eqShape, 2, nullptr);  // LowCut shape
            eqBand0.setProperty(WFSParameterIDs::eqFrequency, static_cast<float>(lowCutFreq), nullptr);
        }
        if (enableHighCut)
        {
            auto eqBand5 = vts.getOutputEQBand(idx, 5);
            eqBand5.setProperty(WFSParameterIDs::eqShape, 4, nullptr);  // HighCut shape
            eqBand5.setProperty(WFSParameterIDs::eqFrequency, static_cast<float>(highCutFreq), nullptr);
        }
    }

    // Advance for next array
    int nextStartOutput = startOutput + N + 1;  // 1-based for selector
    int nextArray = (arrayId % 10) + 1;  // Wrap 10 -> 1

    if (nextStartOutput <= numOutputs)
        startOutputSelector.setSelectedId(nextStartOutput, juce::dontSendNotification);

    arraySelector.setSelectedId(nextArray, juce::dontSendNotification);

    showStatus("Applied " + juce::String(N) + " speakers to Array " + juce::String(arrayId) +
               ". Ready for next array.");

    // Clear calculated positions for next calculation
    calculatedPositions.clear();
    preview->clearPositions();
}

void OutputArrayHelperContent::showStatus(const juce::String& message)
{
    statusLabel.setColour(juce::Label::textColourId, ColorScheme::get().textSecondary);
    statusLabel.setText(message, juce::dontSendNotification);
}

void OutputArrayHelperContent::showError(const juce::String& message)
{
    statusLabel.setColour(juce::Label::textColourId, ColorScheme::get().accentRed);
    statusLabel.setText("Error: " + message, juce::dontSendNotification);
}
