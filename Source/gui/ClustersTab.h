#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "WfsJoystickComponent.h"
#include "sliders/WfsAutoCenterSlider.h"
#include "dials/WfsEndlessDial.h"

/**
 * Clusters Tab Component
 * Management of input clusters with position, rotation, scale, and attenuation controls.
 */
class ClustersTab : public juce::Component,
                    public juce::ListBoxModel,
                    private juce::Timer,
                    private juce::ValueTree::Listener
{
public:
    ClustersTab(WfsParameters& params)
        : parameters(params),
          inputsTree(params.getInputTree()),
          configTree(params.getConfigTree())
    {
        // Add listeners
        inputsTree.addListener(this);
        configTree.addListener(this);

        // ==================== CLUSTER SELECTOR BAR ====================
        for (int i = 0; i < 10; ++i)
        {
            auto* btn = new juce::TextButton(juce::String(i + 1));
            btn->setClickingTogglesState(true);
            btn->setRadioGroupId(1001);
            btn->setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF3A3A3A));
            btn->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF4CAF50));
            btn->onClick = [this, i]() {
                selectCluster(i + 1);
            };
            addAndMakeVisible(btn);
            clusterButtons.add(btn);
        }

        // ==================== ASSIGNED INPUTS PANEL ====================
        addAndMakeVisible(assignedInputsLabel);
        assignedInputsLabel.setText("Assigned Inputs", juce::dontSendNotification);
        assignedInputsLabel.setFont(juce::FontOptions().withHeight(14.0f).withStyle("Bold"));
        assignedInputsLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(inputsList);
        inputsList.setModel(this);
        inputsList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xFF252525));
        inputsList.setRowHeight(24);

        // Reference mode selector
        addAndMakeVisible(referenceModeLabel);
        referenceModeLabel.setText("Reference:", juce::dontSendNotification);
        referenceModeLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(referenceModeSelector);
        referenceModeSelector.addItem("First Input", 1);
        referenceModeSelector.addItem("Barycenter", 2);
        referenceModeSelector.setSelectedId(1, juce::dontSendNotification);
        referenceModeSelector.onChange = [this]() {
            if (selectedCluster > 0)
            {
                int mode = referenceModeSelector.getSelectedId() - 1;
                parameters.getValueTreeState().setClusterParameter(selectedCluster,
                    WFSParameterIDs::clusterReferenceMode, mode);
            }
        };

        // Reference position display
        addAndMakeVisible(refPosLabel);
        refPosLabel.setText("Pos:", juce::dontSendNotification);
        refPosLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

        addAndMakeVisible(refPosXLabel);
        refPosXLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

        addAndMakeVisible(refPosYLabel);
        refPosYLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

        addAndMakeVisible(refPosZLabel);
        refPosZLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

        // Status label (tracking info)
        addAndMakeVisible(statusLabel);
        statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFFF9800));
        statusLabel.setFont(juce::FontOptions().withHeight(12.0f));

        // ==================== CONTROLS PANEL ====================
        // Position joystick label
        addAndMakeVisible(positionLabel);
        positionLabel.setText("Position", juce::dontSendNotification);
        positionLabel.setFont(juce::FontOptions().withHeight(12.0f));
        positionLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        positionLabel.setJustificationType(juce::Justification::centred);

        // Position joystick
        addAndMakeVisible(positionJoystick);
        positionJoystick.setOuterColour(juce::Colour(0xFF3A3A3A));
        positionJoystick.setThumbColour(juce::Colour(0xFF4CAF50));
        positionJoystick.setReportingIntervalHz(50.0);

        // Z slider label
        addAndMakeVisible(zSliderLabel);
        zSliderLabel.setText("Z", juce::dontSendNotification);
        zSliderLabel.setFont(juce::FontOptions().withHeight(12.0f));
        zSliderLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        zSliderLabel.setJustificationType(juce::Justification::centred);

        // Z slider
        zSlider.setTrackColours(juce::Colour(0xFF3A3A3A), juce::Colour(0xFF4CAF50));
        addAndMakeVisible(zSlider);

        // Attenuation slider label
        addAndMakeVisible(attenuationLabel);
        attenuationLabel.setText("Atten", juce::dontSendNotification);
        attenuationLabel.setFont(juce::FontOptions().withHeight(12.0f));
        attenuationLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        attenuationLabel.setJustificationType(juce::Justification::centred);

        // Attenuation slider
        attenuationSlider.setTrackColours(juce::Colour(0xFF3A3A3A), juce::Colour(0xFFFF5722));
        addAndMakeVisible(attenuationSlider);

        // Rotation dial label
        addAndMakeVisible(rotationLabel);
        rotationLabel.setText("Rotation", juce::dontSendNotification);
        rotationLabel.setFont(juce::FontOptions().withHeight(12.0f));
        rotationLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        rotationLabel.setJustificationType(juce::Justification::centred);

        // Rotation dial
        addAndMakeVisible(rotationDial);
        rotationDial.setColours(juce::Colour(0xFF3A3A3A), juce::Colour(0xFF2196F3));

        // Scale joystick label
        addAndMakeVisible(scaleLabel);
        scaleLabel.setText("Scale", juce::dontSendNotification);
        scaleLabel.setFont(juce::FontOptions().withHeight(12.0f));
        scaleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        scaleLabel.setJustificationType(juce::Justification::centred);

        // Scale joystick
        addAndMakeVisible(scaleJoystick);
        scaleJoystick.setOuterColour(juce::Colour(0xFF3A3A3A));
        scaleJoystick.setThumbColour(juce::Colour(0xFF9C27B0));
        scaleJoystick.setReportingIntervalHz(50.0);

        // Plane selector
        addAndMakeVisible(planeLabel);
        planeLabel.setText("Plane:", juce::dontSendNotification);
        planeLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(planeSelector);
        planeSelector.addItem("XY", 1);
        planeSelector.addItem("XZ", 2);
        planeSelector.addItem("YZ", 3);
        planeSelector.setSelectedId(1, juce::dontSendNotification);
        planeSelector.onChange = [this]() {
            currentPlane = static_cast<Plane>(planeSelector.getSelectedId() - 1);
        };

        // ==================== CONTROLS LABEL ====================
        addAndMakeVisible(controlsLabel);
        controlsLabel.setText("Controls", juce::dontSendNotification);
        controlsLabel.setFont(juce::FontOptions().withHeight(14.0f).withStyle("Bold"));
        controlsLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // Start with first cluster selected
        selectCluster(1);
        updateClusterButtonStates();

        // Start timer at 50Hz
        startTimer(20);
    }

    ~ClustersTab() override
    {
        stopTimer();
        inputsTree.removeListener(this);
        configTree.removeListener(this);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF1E1E1E));

        // Draw separator between left and right panels
        auto bounds = getLocalBounds();
        auto leftPanelWidth = bounds.getWidth() / 2;
        g.setColour(juce::Colour(0xFF404040));
        g.drawVerticalLine(leftPanelWidth, 50.0f, bounds.getHeight() - 10.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);

        // ==================== CLUSTER SELECTOR BAR ====================
        auto selectorArea = bounds.removeFromTop(40);
        int buttonWidth = (selectorArea.getWidth() - 90) / 10;
        for (int i = 0; i < 10; ++i)
        {
            clusterButtons[i]->setBounds(selectorArea.removeFromLeft(buttonWidth).reduced(2));
        }

        bounds.removeFromTop(10);

        // Split into left and right panels
        auto leftPanel = bounds.removeFromLeft(bounds.getWidth() / 2).reduced(5, 0);
        auto rightPanel = bounds.reduced(5, 0);

        // ==================== LEFT PANEL - ASSIGNED INPUTS ====================
        assignedInputsLabel.setBounds(leftPanel.removeFromTop(20));
        leftPanel.removeFromTop(5);

        // Inputs list takes most of the space
        auto listArea = leftPanel.removeFromTop(leftPanel.getHeight() - 100);
        inputsList.setBounds(listArea);

        leftPanel.removeFromTop(10);

        // Reference mode selector
        auto refRow = leftPanel.removeFromTop(24);
        referenceModeLabel.setBounds(refRow.removeFromLeft(70));
        referenceModeSelector.setBounds(refRow.removeFromLeft(120));

        leftPanel.removeFromTop(5);

        // Reference position display
        auto posRow = leftPanel.removeFromTop(20);
        refPosLabel.setBounds(posRow.removeFromLeft(35));
        refPosXLabel.setBounds(posRow.removeFromLeft(80));
        refPosYLabel.setBounds(posRow.removeFromLeft(80));
        refPosZLabel.setBounds(posRow.removeFromLeft(80));

        leftPanel.removeFromTop(5);

        // Status label
        statusLabel.setBounds(leftPanel.removeFromTop(20));

        // ==================== RIGHT PANEL - CONTROLS ====================
        controlsLabel.setBounds(rightPanel.removeFromTop(20));
        rightPanel.removeFromTop(10);

        // Position joystick and Z slider
        auto positionRow = rightPanel.removeFromTop(140);
        auto joystickArea = positionRow.removeFromLeft(140);
        positionLabel.setBounds(joystickArea.removeFromTop(16));
        positionJoystick.setBounds(joystickArea.reduced(5));

        positionRow.removeFromLeft(10);

        auto zArea = positionRow.removeFromLeft(40);
        zSliderLabel.setBounds(zArea.removeFromTop(16));
        zSlider.setBounds(zArea.reduced(5));

        positionRow.removeFromLeft(20);

        // Attenuation slider
        auto attenArea = positionRow.removeFromLeft(40);
        attenuationLabel.setBounds(attenArea.removeFromTop(16));
        attenuationSlider.setBounds(attenArea.reduced(5));

        rightPanel.removeFromTop(20);

        // Rotation dial and scale joystick
        auto transformRow = rightPanel.removeFromTop(140);

        auto rotationArea = transformRow.removeFromLeft(100);
        rotationLabel.setBounds(rotationArea.removeFromTop(16));
        rotationDial.setBounds(rotationArea.reduced(10));

        transformRow.removeFromLeft(20);

        auto scaleArea = transformRow.removeFromLeft(120);
        scaleLabel.setBounds(scaleArea.removeFromTop(16));
        scaleJoystick.setBounds(scaleArea.reduced(5));

        rightPanel.removeFromTop(10);

        // Plane selector
        auto planeRow = rightPanel.removeFromTop(24);
        planeLabel.setBounds(planeRow.removeFromLeft(50));
        planeSelector.setBounds(planeRow.removeFromLeft(80));
    }

    // ListBoxModel implementation
    int getNumRows() override
    {
        return static_cast<int>(assignedInputs.size());
    }

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (rowNumber < 0 || rowNumber >= static_cast<int>(assignedInputs.size()))
            return;

        int inputIdx = assignedInputs[static_cast<size_t>(rowNumber)];
        bool isTracked = isInputFullyTracked(inputIdx);
        bool isFirst = (rowNumber == 0);

        // Background
        if (rowIsSelected)
            g.fillAll(juce::Colour(0xFF404040));
        else if (isTracked)
            g.fillAll(juce::Colour(0xFF3D2F00));  // Orange tint for tracked
        else if (isFirst && referenceModeSelector.getSelectedId() == 1)
            g.fillAll(juce::Colour(0xFF2F3D2F));  // Green tint for first input in first-input mode
        else
            g.fillAll(juce::Colour(0xFF2A2A2A));

        // Text
        g.setColour(isTracked ? juce::Colour(0xFFFF9800) : juce::Colours::white);
        juce::String text = "Input " + juce::String(inputIdx + 1);
        if (isTracked)
            text += " [T]";

        g.drawText(text, 10, 0, width - 20, height, juce::Justification::centredLeft);
    }

    void listBoxItemClicked(int row, const juce::MouseEvent&) override
    {
        // Could implement input selection here if needed
        juce::ignoreUnused(row);
    }

private:
    enum class Plane { XY = 0, XZ = 1, YZ = 2 };

    WfsParameters& parameters;
    juce::ValueTree inputsTree;
    juce::ValueTree configTree;

    int selectedCluster = 1;
    Plane currentPlane = Plane::XY;
    float previousDialAngle = 0.0f;

    std::vector<int> assignedInputs;

    // Cluster selector buttons
    juce::OwnedArray<juce::TextButton> clusterButtons;

    // Assigned inputs panel
    juce::Label assignedInputsLabel;
    juce::ListBox inputsList { {}, this };
    juce::Label referenceModeLabel;
    juce::ComboBox referenceModeSelector;
    juce::Label refPosLabel, refPosXLabel, refPosYLabel, refPosZLabel;
    juce::Label statusLabel;

    // Controls panel
    juce::Label controlsLabel;
    juce::Label positionLabel;
    WfsJoystickComponent positionJoystick;
    juce::Label zSliderLabel;
    WfsAutoCenterSlider zSlider { WfsAutoCenterSlider::Orientation::vertical };
    juce::Label attenuationLabel;
    WfsAutoCenterSlider attenuationSlider { WfsAutoCenterSlider::Orientation::vertical };
    juce::Label rotationLabel;
    WfsEndlessDial rotationDial;
    juce::Label scaleLabel;
    WfsJoystickComponent scaleJoystick;
    juce::Label planeLabel;
    juce::ComboBox planeSelector;

    //==========================================================================
    // Cluster Selection and State
    //==========================================================================

    void selectCluster(int clusterIndex)
    {
        selectedCluster = clusterIndex;

        // Update toggle state
        for (int i = 0; i < 10; ++i)
            clusterButtons[i]->setToggleState(i + 1 == clusterIndex, juce::dontSendNotification);

        // Load reference mode
        int mode = static_cast<int>(parameters.getValueTreeState().getClusterParameter(
            clusterIndex, WFSParameterIDs::clusterReferenceMode));
        referenceModeSelector.setSelectedId(mode + 1, juce::dontSendNotification);

        // Reset rotation dial
        rotationDial.setAngle(0.0f);
        previousDialAngle = 0.0f;

        // Update assigned inputs list
        updateAssignedInputsList();
        updateReferencePositionDisplay();
        updateStatusLabel();
    }

    void updateClusterButtonStates()
    {
        int numInputs = parameters.getNumInputChannels();

        for (int c = 1; c <= 10; ++c)
        {
            int inputCount = 0;
            for (int i = 0; i < numInputs; ++i)
            {
                int cluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
                if (cluster == c)
                    inputCount++;
            }

            auto* btn = clusterButtons[c - 1];
            if (inputCount > 0)
            {
                btn->setButtonText(juce::String(c) + " (" + juce::String(inputCount) + ")");
                btn->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
            }
            else
            {
                btn->setButtonText(juce::String(c));
                btn->setColour(juce::TextButton::textColourOffId, juce::Colours::grey);
            }
        }
    }

    void updateAssignedInputsList()
    {
        assignedInputs.clear();

        int numInputs = parameters.getNumInputChannels();
        int trackedInputIdx = -1;

        // First pass: find all inputs in this cluster and identify tracked one
        for (int i = 0; i < numInputs; ++i)
        {
            int cluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
            if (cluster == selectedCluster)
            {
                if (isInputFullyTracked(i))
                    trackedInputIdx = i;
                else
                    assignedInputs.push_back(i);
            }
        }

        // Put tracked input at the front
        if (trackedInputIdx >= 0)
            assignedInputs.insert(assignedInputs.begin(), trackedInputIdx);

        inputsList.updateContent();
        inputsList.repaint();
    }

    void updateReferencePositionDisplay()
    {
        auto [x, y, z] = calculateReferencePoint();
        refPosXLabel.setText("X: " + juce::String(x, 2), juce::dontSendNotification);
        refPosYLabel.setText("Y: " + juce::String(y, 2), juce::dontSendNotification);
        refPosZLabel.setText("Z: " + juce::String(z, 2), juce::dontSendNotification);
    }

    void updateStatusLabel()
    {
        // Check if there's a tracked input in this cluster
        for (int inputIdx : assignedInputs)
        {
            if (isInputFullyTracked(inputIdx))
            {
                statusLabel.setText("Tracking: Input " + juce::String(inputIdx + 1) + " (overrides reference)",
                                    juce::dontSendNotification);
                return;
            }
        }

        if (assignedInputs.empty())
            statusLabel.setText("No inputs assigned", juce::dontSendNotification);
        else
            statusLabel.setText("", juce::dontSendNotification);
    }

    //==========================================================================
    // Tracking Check
    //==========================================================================

    bool isInputFullyTracked(int inputIdx) const
    {
        int globalTracking = static_cast<int>(parameters.getConfigParam("trackingEnabled"));
        int protocolEnabled = static_cast<int>(parameters.getConfigParam("trackingProtocol"));
        int localTracking = static_cast<int>(parameters.getInputParam(inputIdx, "inputTrackingActive"));

        return (globalTracking != 0) && (protocolEnabled != 0) && (localTracking != 0);
    }

    //==========================================================================
    // Reference Point Calculation
    //==========================================================================

    std::tuple<float, float, float> calculateReferencePoint()
    {
        if (assignedInputs.empty())
            return { 0.0f, 0.0f, 0.0f };

        // Priority 1: Tracked input position
        for (int inputIdx : assignedInputs)
        {
            if (isInputFullyTracked(inputIdx))
                return getInputPosition(inputIdx);
        }

        // Priority 2: First input or barycenter per mode
        int mode = referenceModeSelector.getSelectedId() - 1;

        if (mode == 0)  // First Input
        {
            return getInputPosition(assignedInputs[0]);
        }
        else  // Barycenter
        {
            return calculateBarycenter();
        }
    }

    std::tuple<float, float, float> calculateBarycenter()
    {
        if (assignedInputs.empty())
            return { 0.0f, 0.0f, 0.0f };

        float sumX = 0, sumY = 0, sumZ = 0;
        for (int inputIdx : assignedInputs)
        {
            auto [x, y, z] = getInputPosition(inputIdx);
            sumX += x;
            sumY += y;
            sumZ += z;
        }

        float n = static_cast<float>(assignedInputs.size());
        return { sumX / n, sumY / n, sumZ / n };
    }

    std::tuple<float, float, float> getInputPosition(int inputIdx) const
    {
        float x = static_cast<float>(parameters.getInputParam(inputIdx, "inputPositionX"));
        float y = static_cast<float>(parameters.getInputParam(inputIdx, "inputPositionY"));
        float z = static_cast<float>(parameters.getInputParam(inputIdx, "inputPositionZ"));
        return { x, y, z };
    }

    void setInputPosition(int inputIdx, float x, float y, float z)
    {
        parameters.setInputParam(inputIdx, "inputPositionX", x);
        parameters.setInputParam(inputIdx, "inputPositionY", y);
        parameters.setInputParam(inputIdx, "inputPositionZ", z);
    }

    std::tuple<float, float, float> getInputOffset(int inputIdx) const
    {
        float x = static_cast<float>(parameters.getInputParam(inputIdx, "inputOffsetX"));
        float y = static_cast<float>(parameters.getInputParam(inputIdx, "inputOffsetY"));
        float z = static_cast<float>(parameters.getInputParam(inputIdx, "inputOffsetZ"));
        return { x, y, z };
    }

    void setInputOffset(int inputIdx, float x, float y, float z)
    {
        parameters.setInputParam(inputIdx, "inputOffsetX", x);
        parameters.setInputParam(inputIdx, "inputOffsetY", y);
        parameters.setInputParam(inputIdx, "inputOffsetZ", z);
    }

    //==========================================================================
    // Transformation Algorithms
    //==========================================================================

    void applyPositionDelta(float dx, float dy, float dz)
    {
        if (assignedInputs.empty())
            return;

        // Find tracked input
        int trackedIdx = -1;
        for (int inputIdx : assignedInputs)
        {
            if (isInputFullyTracked(inputIdx))
            {
                trackedIdx = inputIdx;
                break;
            }
        }

        if (trackedIdx >= 0)
        {
            // Move tracked input's OFFSET (not position)
            auto [ox, oy, oz] = getInputOffset(trackedIdx);
            setInputOffset(trackedIdx, ox + dx, oy + dy, oz + dz);
        }
        else
        {
            // Move all inputs' positions
            for (int inputIdx : assignedInputs)
            {
                auto [px, py, pz] = getInputPosition(inputIdx);
                setInputPosition(inputIdx, px + dx, py + dy, pz + dz);
            }
        }
    }

    void applyAttenuationDelta(float deltaDB)
    {
        for (int inputIdx : assignedInputs)
        {
            float current = static_cast<float>(parameters.getInputParam(inputIdx, "inputAttenuation"));
            float newAtten = juce::jlimit(-92.0f, 0.0f, current + deltaDB);
            parameters.setInputParam(inputIdx, "inputAttenuation", newAtten);
        }
    }

    void applyRotationDelta(float angleDeg)
    {
        if (assignedInputs.empty())
            return;

        auto [refX, refY, refZ] = calculateReferencePoint();

        float rad = juce::degreesToRadians(angleDeg);
        float cosA = std::cos(rad);
        float sinA = std::sin(rad);

        for (int inputIdx : assignedInputs)
        {
            auto [px, py, pz] = getInputPosition(inputIdx);
            float newX = px, newY = py, newZ = pz;

            switch (currentPlane)
            {
                case Plane::XY:
                    newX = refX + (px - refX) * cosA - (py - refY) * sinA;
                    newY = refY + (px - refX) * sinA + (py - refY) * cosA;
                    break;
                case Plane::XZ:
                    newX = refX + (px - refX) * cosA - (pz - refZ) * sinA;
                    newZ = refZ + (px - refX) * sinA + (pz - refZ) * cosA;
                    break;
                case Plane::YZ:
                    newY = refY + (py - refY) * cosA - (pz - refZ) * sinA;
                    newZ = refZ + (py - refY) * sinA + (pz - refZ) * cosA;
                    break;
            }

            setInputPosition(inputIdx, newX, newY, newZ);
        }
    }

    void applyScaleDelta(float scaleX, float scaleY)
    {
        if (assignedInputs.empty())
            return;

        auto [refX, refY, refZ] = calculateReferencePoint();

        for (int inputIdx : assignedInputs)
        {
            auto [px, py, pz] = getInputPosition(inputIdx);
            float newX = px, newY = py, newZ = pz;

            switch (currentPlane)
            {
                case Plane::XY:
                    newX = refX + (px - refX) * scaleX;
                    newY = refY + (py - refY) * scaleY;
                    break;
                case Plane::XZ:
                    newX = refX + (px - refX) * scaleX;
                    newZ = refZ + (pz - refZ) * scaleY;
                    break;
                case Plane::YZ:
                    newY = refY + (py - refY) * scaleX;
                    newZ = refZ + (pz - refZ) * scaleY;
                    break;
            }

            setInputPosition(inputIdx, newX, newY, newZ);
        }
    }

    //==========================================================================
    // Timer Callback (50Hz)
    //==========================================================================

    void timerCallback() override
    {
        if (selectedCluster < 1 || assignedInputs.empty())
            return;

        // Position joystick (auto-centers, gives -1..1 values)
        auto [jx, jy] = positionJoystick.getCurrentPosition();
        if (jx != 0.0f || jy != 0.0f)
            applyPositionDelta(jx * 0.05f, jy * 0.05f, 0.0f);

        // Z slider (auto-centers)
        float zVal = zSlider.getValue();
        if (zVal != 0.0f)
            applyPositionDelta(0.0f, 0.0f, zVal * 0.05f);

        // Attenuation slider (auto-centers)
        float attenVal = attenuationSlider.getValue();
        if (attenVal != 0.0f)
            applyAttenuationDelta(attenVal * 0.5f);  // 0.5 dB per full deflection per tick

        // Rotation dial (1:1, calculate delta from previous)
        float currentAngle = rotationDial.getAngle();
        float angleDelta = currentAngle - previousDialAngle;
        // Handle wrap-around at +/-180
        if (angleDelta > 180.0f) angleDelta -= 360.0f;
        if (angleDelta < -180.0f) angleDelta += 360.0f;
        if (angleDelta != 0.0f)
            applyRotationDelta(angleDelta);
        previousDialAngle = currentAngle;

        // Scale joystick (auto-centers, gives -1..1)
        auto [sx, sy] = scaleJoystick.getCurrentPosition();
        if (sx != 0.0f || sy != 0.0f)
        {
            float scaleX = 1.0f + sx * 0.02f;  // +/-2% per full deflection per tick
            float scaleY = 1.0f + sy * 0.02f;
            applyScaleDelta(scaleX, scaleY);
        }

        // Update reference position display periodically
        updateReferencePositionDisplay();
    }

    //==========================================================================
    // ValueTree::Listener
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                  const juce::Identifier& property) override
    {
        juce::ignoreUnused(treeWhosePropertyHasChanged);

        // Update when input cluster assignments change
        if (property == WFSParameterIDs::inputCluster)
        {
            juce::MessageManager::callAsync([this]() {
                updateClusterButtonStates();
                updateAssignedInputsList();
                updateStatusLabel();
            });
        }
        // Update when tracking state changes
        else if (property == WFSParameterIDs::inputTrackingActive ||
                 property == WFSParameterIDs::trackingEnabled ||
                 property == WFSParameterIDs::trackingProtocol)
        {
            juce::MessageManager::callAsync([this]() {
                updateAssignedInputsList();
                updateStatusLabel();
            });
        }
    }

    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override
    {
        juce::MessageManager::callAsync([this]() {
            updateClusterButtonStates();
            updateAssignedInputsList();
        });
    }

    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override
    {
        juce::MessageManager::callAsync([this]() {
            updateClusterButtonStates();
            updateAssignedInputsList();
        });
    }

    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree&) override {}

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClustersTab)
};
