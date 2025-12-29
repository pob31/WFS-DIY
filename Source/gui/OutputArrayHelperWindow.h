#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Helpers/ArrayGeometryCalculator.h"

//==============================================================================
// Forward declarations
class OutputArrayHelperContent;

//==============================================================================
/**
 * Array preset types available in the helper
 */
enum class ArrayPresetType
{
    NearFieldStraight = 0,
    NearFieldCurved,
    MainRoomStraight,
    SubBass,
    Surround,
    DelayLine,
    Circle
};

//==============================================================================
/**
 * Geometry input method for array positioning
 */
enum class GeometryMethod
{
    CenterSpacing,
    Endpoints
};

//==============================================================================
/**
 * Configuration structure for each array preset
 */
struct ArrayPresetConfig
{
    ArrayPresetType type;
    juce::String name;

    // Geometry capabilities
    bool supportsCenterSpacing;
    bool supportsEndpoints;
    bool supportsCurve;       // For curved arrays (sag parameter)
    bool supportsCircle;      // For circle arrays (radius, start angle)
    bool supportsSurround;    // For surround pairs

    // Default acoustic settings
    bool lsAttenEnable;
    bool frEnable;                // Floor Reflections enable
    float hfDamping;              // dB/m
    float hParallax;              // meters
    float vParallax;              // meters
    int distanceAttenPercent;     // 0-200%

    // EQ defaults
    bool hasLowCut;
    int lowCutFreq;               // Hz
    bool hasHighCut;
    int highCutFreq;              // Hz
};

//==============================================================================
/**
 * Preview component showing the stage and calculated speaker positions
 */
class ArrayPreviewComponent : public juce::Component
{
public:
    ArrayPreviewComponent(WfsParameters& params);

    void paint(juce::Graphics& g) override;
    void resized() override;

    /** Set the calculated positions to display */
    void setPositions(const std::vector<SpeakerPosition>& positions);

    /** Clear the displayed positions */
    void clearPositions();

    /** Set current preset info for audience label display */
    void setPresetInfo(ArrayPresetType preset, bool circleInward, float radius = 5.0f);

private:
    WfsParameters& parameters;
    std::vector<SpeakerPosition> speakerPositions;

    // Preset info for audience label
    ArrayPresetType currentPreset = ArrayPresetType::NearFieldStraight;
    bool circleFacingInward = true;
    float circleRadius = 5.0f;

    // Coordinate transformation
    float scale = 1.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    void calculateTransform();
    juce::Point<float> stageToScreen(float stageX, float stageY) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrayPreviewComponent)
};

//==============================================================================
/**
 * Main content component for the Output Array Helper window
 */
class OutputArrayHelperContent : public juce::Component
{
public:
    OutputArrayHelperContent(WfsParameters& params);
    ~OutputArrayHelperContent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    WfsParameters& parameters;

    //==========================================================================
    // Preset section
    juce::Label presetLabel;
    juce::ComboBox presetSelector;

    //==========================================================================
    // Geometry section
    juce::GroupComponent geometryGroup;
    juce::ToggleButton centerSpacingRadio;
    juce::ToggleButton endpointsRadio;

    // Common geometry fields
    juce::Label numSpeakersLabel;
    juce::TextEditor numSpeakersEditor;
    juce::Label zPositionLabel;
    juce::TextEditor zPositionEditor;
    juce::Label orientationLabel;
    juce::TextEditor orientationEditor;

    // Center + Spacing method
    juce::Label centerXLabel, centerYLabel;
    juce::TextEditor centerXEditor, centerYEditor;
    juce::Label spacingLabel;
    juce::TextEditor spacingEditor;

    // Endpoints method
    juce::Label startXLabel, startYLabel;
    juce::TextEditor startXEditor, startYEditor;
    juce::Label endXLabel, endYLabel;
    juce::TextEditor endXEditor, endYEditor;

    // Curved array specific
    juce::Label sagLabel;
    juce::TextEditor sagEditor;

    // Circle array specific
    juce::Label radiusLabel, startAngleLabel;
    juce::TextEditor radiusEditor, startAngleEditor;
    juce::ToggleButton facingInwardRadio;
    juce::ToggleButton facingOutwardRadio;

    // Surround specific
    juce::Label widthLabel, yStartLabel, yEndLabel;
    juce::TextEditor widthEditor, yStartEditor, yEndEditor;

    // Delay line specific
    juce::ToggleButton frontFacingRadio;
    juce::ToggleButton backFacingRadio;

    //==========================================================================
    // Acoustic defaults section
    juce::GroupComponent acousticGroup;
    juce::ToggleButton lsEnableButton;
    juce::ToggleButton frEnableButton;
    juce::Label hfDampingLabel;
    juce::TextEditor hfDampingEditor;
    juce::Label hParallaxLabel, vParallaxLabel;
    juce::TextEditor hParallaxEditor, vParallaxEditor;
    juce::Label distanceAttenLabel;
    juce::TextEditor distanceAttenEditor;

    // EQ section
    juce::ToggleButton lowCutEnableButton;
    juce::Label lowCutFreqLabel;
    juce::TextEditor lowCutFreqEditor;
    juce::ToggleButton highCutEnableButton;
    juce::Label highCutFreqLabel;
    juce::TextEditor highCutFreqEditor;

    //==========================================================================
    // Target section
    juce::GroupComponent targetGroup;
    juce::Label arrayLabel;
    juce::ComboBox arraySelector;
    juce::Label startOutputLabel;
    juce::ComboBox startOutputSelector;

    //==========================================================================
    // Preview
    std::unique_ptr<ArrayPreviewComponent> preview;

    //==========================================================================
    // Buttons
    juce::TextButton applyButton;
    juce::TextButton closeButton;

    // Status
    juce::Label statusLabel;

    //==========================================================================
    // State
    std::vector<SpeakerPosition> calculatedPositions;
    ArrayPresetType currentPreset = ArrayPresetType::NearFieldStraight;

    //==========================================================================
    // Preset configurations
    static const std::array<ArrayPresetConfig, 7> presetConfigs;

    //==========================================================================
    // Methods
    void setupPresetSelector();
    void setupGeometrySection();
    void setupAcousticSection();
    void setupTargetSection();
    void setupButtons();

    void onPresetChanged();
    void updateGeometryVisibility();
    void loadPresetDefaults(ArrayPresetType preset);

    void calculatePositions();
    void autoCalculatePreview();  // Auto-update preview on parameter change
    void applyToOutputs();

    void showStatus(const juce::String& message);
    void showError(const juce::String& message);

    // Layout helpers
    void layoutGeometrySection(juce::Rectangle<int>& area);
    void layoutAcousticSection(juce::Rectangle<int>& area);
    void layoutTargetSection(juce::Rectangle<int>& area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputArrayHelperContent)
};

//==============================================================================
/**
 * Output Array Position Helper Window
 *
 * A wizard-style dialog for quickly configuring speaker array positions
 * with preset acoustic defaults.
 */
class OutputArrayHelperWindow : public juce::DocumentWindow
{
public:
    OutputArrayHelperWindow(WfsParameters& params)
        : DocumentWindow("Wizard of OutZ",
                         juce::Colour(0xFF1E1E1E),
                         DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);

        content = std::make_unique<OutputArrayHelperContent>(params);
        setContentOwned(content.get(), false);

        // Set size with display awareness
        const int preferredWidth = 900;
        const int preferredHeight = 700;

        auto& displays = juce::Desktop::getInstance().getDisplays();
        const auto* displayPtr = displays.getPrimaryDisplay();
        juce::Rectangle<int> userArea = (displayPtr != nullptr && !displayPtr->userArea.isEmpty())
            ? displayPtr->userArea
            : displays.getTotalBounds(true);

        const int margin = 40;
        const int windowWidth = juce::jmin(preferredWidth, userArea.getWidth() - margin);
        const int windowHeight = juce::jmin(preferredHeight, userArea.getHeight() - margin);

        setResizeLimits(700, 500, userArea.getWidth(), userArea.getHeight());
        centreWithSize(windowWidth, windowHeight);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

private:
    std::unique_ptr<OutputArrayHelperContent> content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputArrayHelperWindow)
};
