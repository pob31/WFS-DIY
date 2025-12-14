#include "OutputArrayHelperWindow.h"

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

    // Background
    g.fillAll(juce::Colour(0xFF1A1A1A));

    // Get stage dimensions from parameters
    float stageWidth = parameters.getConfigParam("stageWidth").isVoid() ? 20.0f :
                       static_cast<float>(parameters.getConfigParam("stageWidth"));
    float stageDepth = parameters.getConfigParam("stageDepth").isVoid() ? 15.0f :
                       static_cast<float>(parameters.getConfigParam("stageDepth"));
    float originX = parameters.getConfigParam("originWidth").isVoid() ? stageWidth / 2.0f :
                    static_cast<float>(parameters.getConfigParam("originWidth"));
    float originY = parameters.getConfigParam("originDepth").isVoid() ? 0.0f :
                    static_cast<float>(parameters.getConfigParam("originDepth"));

    // Calculate transform to fit stage in view with padding
    const float padding = 20.0f;
    float viewWidth = bounds.getWidth() - padding * 2;
    float viewHeight = bounds.getHeight() - padding * 2;

    float scaleX = viewWidth / stageWidth;
    float scaleY = viewHeight / stageDepth;
    scale = juce::jmin(scaleX, scaleY);

    // Center the stage in the view
    float scaledWidth = stageWidth * scale;
    float scaledHeight = stageDepth * scale;
    offsetX = padding + (viewWidth - scaledWidth) / 2.0f + originX * scale;
    offsetY = padding + (viewHeight - scaledHeight) / 2.0f + originY * scale;

    // Draw stage bounds
    auto stageTopLeft = stageToScreen(-originX, stageDepth - originY);
    auto stageBottomRight = stageToScreen(stageWidth - originX, -originY);

    juce::Rectangle<float> stageRect(stageTopLeft.x, stageTopLeft.y,
                                      stageBottomRight.x - stageTopLeft.x,
                                      stageBottomRight.y - stageTopLeft.y);

    g.setColour(juce::Colour(0xFF303030));
    g.fillRect(stageRect);
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawRect(stageRect, 1.0f);

    // Draw grid lines (1m spacing)
    g.setColour(juce::Colour(0xFF404040));
    for (float x = -originX; x <= stageWidth - originX; x += 1.0f)
    {
        auto top = stageToScreen(x, stageDepth - originY);
        auto bottom = stageToScreen(x, -originY);
        g.drawLine(top.x, top.y, bottom.x, bottom.y, 0.5f);
    }
    for (float y = -originY; y <= stageDepth - originY; y += 1.0f)
    {
        auto left = stageToScreen(-originX, y);
        auto right = stageToScreen(stageWidth - originX, y);
        g.drawLine(left.x, left.y, right.x, right.y, 0.5f);
    }

    // Draw origin marker
    auto origin = stageToScreen(0, 0);
    g.setColour(juce::Colours::white);
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
        g.setColour(juce::Colour(0xFF4080FF));
        g.fillEllipse(screenPos.x - speakerRadius, screenPos.y - speakerRadius,
                      speakerRadius * 2, speakerRadius * 2);

        // Draw orientation arrow
        // 0 degrees = facing audience (toward negative Y in stage coords, positive Y on screen)
        float orientRad = juce::degreesToRadians(pos.orientation);
        float arrowDx = std::sin(orientRad) * arrowLength;
        float arrowDy = std::cos(orientRad) * arrowLength;  // Positive because screen Y is inverted

        g.setColour(juce::Colours::white);
        g.drawArrow(juce::Line<float>(screenPos.x, screenPos.y,
                                       screenPos.x + arrowDx, screenPos.y + arrowDy),
                    2.0f, 6.0f, 6.0f);

        // Draw speaker number
        g.setColour(juce::Colours::white);
        g.setFont(10.0f);
        g.drawText(juce::String(i + 1),
                   juce::Rectangle<float>(screenPos.x - 15, screenPos.y - speakerRadius - 15, 30, 12),
                   juce::Justification::centred);
    }

    // Draw "Audience" label at bottom
    g.setColour(juce::Colours::grey);
    g.setFont(12.0f);
    g.drawText("Audience", bounds.removeFromBottom(20), juce::Justification::centred);
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
    presetLabel.setColour(juce::Label::textColourId, juce::Colours::white);

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
    geometryGroup.setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xFF606060));
    geometryGroup.setColour(juce::GroupComponent::textColourId, juce::Colours::white);

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
        label.setColour(juce::Label::textColourId, juce::Colours::white);
    };

    auto setupEditor = [this](juce::TextEditor& editor, const juce::String& defaultText = "") {
        addAndMakeVisible(editor);
        editor.setText(defaultText);
        editor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF2A2A2A));
        editor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
        editor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xFF606060));
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
    acousticGroup.setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xFF606060));
    acousticGroup.setColour(juce::GroupComponent::textColourId, juce::Colours::white);

    auto setupLabel = [this](juce::Label& label, const juce::String& text) {
        addAndMakeVisible(label);
        label.setText(text, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
    };

    auto setupEditor = [this](juce::TextEditor& editor, const juce::String& defaultText = "") {
        addAndMakeVisible(editor);
        editor.setText(defaultText);
        editor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF2A2A2A));
        editor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
        editor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xFF606060));
    };

    addAndMakeVisible(lsEnableButton);
    lsEnableButton.setButtonText("Live Source");
    lsEnableButton.setToggleState(true, juce::dontSendNotification);

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
    lowCutEnableButton.setButtonText("Low Cut");
    lowCutEnableButton.setToggleState(true, juce::dontSendNotification);

    setupLabel(lowCutFreqLabel, "Freq (Hz):");
    setupEditor(lowCutFreqEditor, "80");

    addAndMakeVisible(highCutEnableButton);
    highCutEnableButton.setButtonText("High Cut");
    highCutEnableButton.setToggleState(false, juce::dontSendNotification);

    setupLabel(highCutFreqLabel, "Freq (Hz):");
    setupEditor(highCutFreqEditor, "300");
}

void OutputArrayHelperContent::setupTargetSection()
{
    addAndMakeVisible(targetGroup);
    targetGroup.setText("Target");
    targetGroup.setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xFF606060));
    targetGroup.setColour(juce::GroupComponent::textColourId, juce::Colours::white);

    addAndMakeVisible(arrayLabel);
    arrayLabel.setText("Array:", juce::dontSendNotification);
    arrayLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    addAndMakeVisible(arraySelector);
    for (int i = 1; i <= 10; ++i)
        arraySelector.addItem("Array " + juce::String(i), i);
    arraySelector.setSelectedId(1, juce::dontSendNotification);

    addAndMakeVisible(startOutputLabel);
    startOutputLabel.setText("Starting Output:", juce::dontSendNotification);
    startOutputLabel.setColour(juce::Label::textColourId, juce::Colours::white);

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
    applyButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF338C33));
    applyButton.onClick = [this]() { applyToOutputs(); };

    addAndMakeVisible(closeButton);
    closeButton.setButtonText("Close");
    closeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF8C3333));
    closeButton.onClick = [this]() {
        if (auto* window = findParentComponentOfClass<OutputArrayHelperWindow>())
            window->setVisible(false);
    };

    addAndMakeVisible(statusLabel);
    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
}

void OutputArrayHelperContent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF252525));
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
    const int labelWidth = 110;
    const int editorWidth = 70;
    const int spacing = 5;

    auto& config = presetConfigs[static_cast<int>(currentPreset)];

    // Calculate height needed
    int height = 30;  // Header
    height += rowHeight;  // Radio buttons
    height += rowHeight;  // N Speakers
    height += rowHeight;  // Z Height

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

    // Radio buttons
    if (config.supportsCenterSpacing && config.supportsEndpoints)
    {
        auto radioRow = content.removeFromTop(rowHeight);
        centerSpacingRadio.setBounds(radioRow.removeFromLeft(150));
        endpointsRadio.setBounds(radioRow.removeFromLeft(150));
        centerSpacingRadio.setVisible(true);
        endpointsRadio.setVisible(true);
    }
    else
    {
        centerSpacingRadio.setVisible(false);
        endpointsRadio.setVisible(false);
    }

    // N Speakers row
    auto row = content.removeFromTop(rowHeight);
    numSpeakersLabel.setBounds(row.removeFromLeft(labelWidth));
    numSpeakersEditor.setBounds(row.removeFromLeft(editorWidth));
    row.removeFromLeft(spacing * 2);
    zPositionLabel.setBounds(row.removeFromLeft(labelWidth));
    zPositionEditor.setBounds(row.removeFromLeft(editorWidth));

    // Orientation (for straight arrays)
    if (!config.supportsCircle && !config.supportsSurround)
    {
        row = content.removeFromTop(rowHeight);
        orientationLabel.setBounds(row.removeFromLeft(labelWidth));
        orientationEditor.setBounds(row.removeFromLeft(editorWidth));
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
        centerXLabel.setBounds(row.removeFromLeft(labelWidth));
        centerXEditor.setBounds(row.removeFromLeft(editorWidth));
        row.removeFromLeft(spacing * 2);
        centerYLabel.setBounds(row.removeFromLeft(labelWidth));
        centerYEditor.setBounds(row.removeFromLeft(editorWidth));

        row = content.removeFromTop(rowHeight);
        spacingLabel.setBounds(row.removeFromLeft(labelWidth));
        spacingEditor.setBounds(row.removeFromLeft(editorWidth));
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
        startXLabel.setBounds(row.removeFromLeft(labelWidth));
        startXEditor.setBounds(row.removeFromLeft(editorWidth));
        row.removeFromLeft(spacing * 2);
        startYLabel.setBounds(row.removeFromLeft(labelWidth));
        startYEditor.setBounds(row.removeFromLeft(editorWidth));

        row = content.removeFromTop(rowHeight);
        endXLabel.setBounds(row.removeFromLeft(labelWidth));
        endXEditor.setBounds(row.removeFromLeft(editorWidth));
        row.removeFromLeft(spacing * 2);
        endYLabel.setBounds(row.removeFromLeft(labelWidth));
        endYEditor.setBounds(row.removeFromLeft(editorWidth));
    }

    // Curved array (sag)
    sagLabel.setVisible(config.supportsCurve);
    sagEditor.setVisible(config.supportsCurve);
    if (config.supportsCurve)
    {
        row = content.removeFromTop(rowHeight);
        sagLabel.setBounds(row.removeFromLeft(labelWidth));
        sagEditor.setBounds(row.removeFromLeft(editorWidth));
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
        centerXLabel.setBounds(row.removeFromLeft(labelWidth));
        centerXEditor.setBounds(row.removeFromLeft(editorWidth));
        row.removeFromLeft(spacing * 2);
        centerYLabel.setBounds(row.removeFromLeft(labelWidth));
        centerYEditor.setBounds(row.removeFromLeft(editorWidth));
        centerXLabel.setVisible(true);
        centerXEditor.setVisible(true);
        centerYLabel.setVisible(true);
        centerYEditor.setVisible(true);

        row = content.removeFromTop(rowHeight);
        radiusLabel.setBounds(row.removeFromLeft(labelWidth));
        radiusEditor.setBounds(row.removeFromLeft(editorWidth));
        row.removeFromLeft(spacing * 2);
        startAngleLabel.setBounds(row.removeFromLeft(labelWidth));
        startAngleEditor.setBounds(row.removeFromLeft(editorWidth));

        row = content.removeFromTop(rowHeight);
        facingInwardRadio.setBounds(row.removeFromLeft(150));
        facingOutwardRadio.setBounds(row.removeFromLeft(150));
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
        centerXLabel.setBounds(row.removeFromLeft(labelWidth));
        centerXEditor.setBounds(row.removeFromLeft(editorWidth));
        row.removeFromLeft(spacing * 2);
        widthLabel.setBounds(row.removeFromLeft(labelWidth));
        widthEditor.setBounds(row.removeFromLeft(editorWidth));
        centerXLabel.setVisible(true);
        centerXEditor.setVisible(true);

        row = content.removeFromTop(rowHeight);
        yStartLabel.setBounds(row.removeFromLeft(labelWidth));
        yStartEditor.setBounds(row.removeFromLeft(editorWidth));
        row.removeFromLeft(spacing * 2);
        yEndLabel.setBounds(row.removeFromLeft(labelWidth));
        yEndEditor.setBounds(row.removeFromLeft(editorWidth));

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
        frontFacingRadio.setBounds(row.removeFromLeft(150));
        backFacingRadio.setBounds(row.removeFromLeft(150));
    }
}

void OutputArrayHelperContent::layoutAcousticSection(juce::Rectangle<int>& area)
{
    const int rowHeight = 26;
    const int labelWidth = 130;
    const int editorWidth = 60;
    const int spacing = 5;

    int height = 30 + rowHeight * 4 + 15;
    auto section = area.removeFromTop(height);
    acousticGroup.setBounds(section);

    auto content = section.reduced(10, 20);
    content.removeFromTop(10);

    // Row 1: LS Enable, HF Damping
    auto row = content.removeFromTop(rowHeight);
    lsEnableButton.setBounds(row.removeFromLeft(120));
    row.removeFromLeft(spacing * 2);
    hfDampingLabel.setBounds(row.removeFromLeft(labelWidth));
    hfDampingEditor.setBounds(row.removeFromLeft(editorWidth));

    // Row 2: Parallax
    row = content.removeFromTop(rowHeight);
    hParallaxLabel.setBounds(row.removeFromLeft(labelWidth));
    hParallaxEditor.setBounds(row.removeFromLeft(editorWidth));
    row.removeFromLeft(spacing * 2);
    vParallaxLabel.setBounds(row.removeFromLeft(labelWidth));
    vParallaxEditor.setBounds(row.removeFromLeft(editorWidth));

    // Row 3: Distance Atten
    row = content.removeFromTop(rowHeight);
    distanceAttenLabel.setBounds(row.removeFromLeft(labelWidth));
    distanceAttenEditor.setBounds(row.removeFromLeft(editorWidth));

    // Row 4: EQ
    row = content.removeFromTop(rowHeight);
    lowCutEnableButton.setBounds(row.removeFromLeft(80));
    lowCutFreqLabel.setBounds(row.removeFromLeft(60));
    lowCutFreqEditor.setBounds(row.removeFromLeft(editorWidth));
    row.removeFromLeft(spacing * 2);
    highCutEnableButton.setBounds(row.removeFromLeft(80));
    highCutFreqLabel.setBounds(row.removeFromLeft(60));
    highCutFreqEditor.setBounds(row.removeFromLeft(editorWidth));
}

void OutputArrayHelperContent::layoutTargetSection(juce::Rectangle<int>& area)
{
    const int rowHeight = 26;
    const int labelWidth = 110;
    const int selectorWidth = 100;

    int height = 30 + rowHeight + 15;
    auto section = area.removeFromTop(height);
    targetGroup.setBounds(section);

    auto content = section.reduced(10, 20);
    content.removeFromTop(10);

    auto row = content.removeFromTop(rowHeight);
    arrayLabel.setBounds(row.removeFromLeft(labelWidth));
    arraySelector.setBounds(row.removeFromLeft(selectorWidth));
    row.removeFromLeft(20);
    startOutputLabel.setBounds(row.removeFromLeft(labelWidth));
    startOutputSelector.setBounds(row.removeFromLeft(selectorWidth));
}

void OutputArrayHelperContent::onPresetChanged()
{
    int selectedId = presetSelector.getSelectedId();
    if (selectedId < 1 || selectedId > static_cast<int>(presetConfigs.size()))
        return;

    currentPreset = static_cast<ArrayPresetType>(selectedId - 1);
    loadPresetDefaults(currentPreset);
    updateGeometryVisibility();
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
            centerYEditor.setText("5");
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
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    statusLabel.setText(message, juce::dontSendNotification);
}

void OutputArrayHelperContent::showError(const juce::String& message)
{
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    statusLabel.setText("Error: " + message, juce::dontSendNotification);
}
