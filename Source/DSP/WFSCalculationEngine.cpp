#include "WFSCalculationEngine.h"

using namespace WFSParameterIDs;
using namespace WFSParameterDefaults;

//==============================================================================
WFSCalculationEngine::WFSCalculationEngine (WFSValueTreeState& state)
    : valueTreeState (state)
{
    numInputs = maxInputChannels;
    numOutputs = maxOutputChannels;

    // Reserve space for positions
    listenerPositions.resize (static_cast<size_t> (numOutputs));
    speakerPositions.resize (static_cast<size_t> (numOutputs));
    inputPositions.resize (static_cast<size_t> (numInputs));

    // Reserve space for matrix results
    const size_t matrixSize = static_cast<size_t> (numInputs * numOutputs);
    delayTimesMs.resize (matrixSize, 0.0f);
    levels.resize (matrixSize, 0.0f);
    hfAttenuationDb.resize (matrixSize, 0.0f);

    // Calculate initial positions
    recalculateAllListenerPositions();
    recalculateAllInputPositions();

    // Initial matrix calculation
    recalculateMatrix();

    // Listen to parameter changes
    valueTreeState.addListener (this);
}

WFSCalculationEngine::~WFSCalculationEngine()
{
    valueTreeState.removeListener (this);
}

//==============================================================================
// Position Access
//==============================================================================

WFSCalculationEngine::Position WFSCalculationEngine::getListenerPosition (int outputIndex) const
{
    if (outputIndex < 0 || outputIndex >= numOutputs)
        return {};

    const juce::ScopedLock sl (positionLock);
    return listenerPositions[static_cast<size_t> (outputIndex)];
}

WFSCalculationEngine::Position WFSCalculationEngine::getSpeakerPosition (int outputIndex) const
{
    if (outputIndex < 0 || outputIndex >= numOutputs)
        return {};

    const juce::ScopedLock sl (positionLock);
    return speakerPositions[static_cast<size_t> (outputIndex)];
}

WFSCalculationEngine::Position WFSCalculationEngine::getInputPosition (int inputIndex) const
{
    if (inputIndex < 0 || inputIndex >= numInputs)
        return {};

    const juce::ScopedLock sl (positionLock);
    return inputPositions[static_cast<size_t> (inputIndex)];
}

//==============================================================================
// Position Recalculation
//==============================================================================

void WFSCalculationEngine::recalculateAllListenerPositions()
{
    const juce::ScopedLock sl (positionLock);

    for (int i = 0; i < numOutputs; ++i)
    {
        updateSpeakerPosition (i);
        recalculateListenerPosition (i);
    }
}

void WFSCalculationEngine::recalculateAllInputPositions()
{
    const juce::ScopedLock sl (positionLock);

    for (int i = 0; i < numInputs; ++i)
    {
        updateInputPosition (i);
    }
}

void WFSCalculationEngine::recalculateListenerPosition (int outputIndex)
{
    // Note: Assumes positionLock is already held

    const auto& speaker = speakerPositions[static_cast<size_t> (outputIndex)];

    auto positionSection = valueTreeState.getOutputPositionSection (outputIndex);
    auto optionsSection = valueTreeState.getOutputOptionsSection (outputIndex);

    int orientationDeg = positionSection.getProperty (outputOrientation, 0);
    float hParallax = optionsSection.getProperty (outputHparallax, 0.0f);
    float vParallax = optionsSection.getProperty (outputVparallax, 0.0f);

    float orientationRad = static_cast<float> (orientationDeg) * (juce::MathConstants<float>::pi / 180.0f);

    // 0° = facing audience (toward -Y)
    Position& listener = listenerPositions[static_cast<size_t> (outputIndex)];
    listener.x = speaker.x + hParallax * std::sin (orientationRad);
    listener.y = speaker.y - hParallax * std::cos (orientationRad);
    listener.z = speaker.z + vParallax;
}

void WFSCalculationEngine::updateSpeakerPosition (int outputIndex)
{
    // Note: Assumes positionLock is already held

    auto positionSection = valueTreeState.getOutputPositionSection (outputIndex);

    Position& speaker = speakerPositions[static_cast<size_t> (outputIndex)];
    speaker.x = positionSection.getProperty (outputPositionX, 0.0f);
    speaker.y = positionSection.getProperty (outputPositionY, 0.0f);
    speaker.z = positionSection.getProperty (outputPositionZ, 0.0f);
}

void WFSCalculationEngine::updateInputPosition (int inputIndex)
{
    // Note: Assumes positionLock is already held

    auto positionSection = valueTreeState.getInputPositionSection (inputIndex);

    Position& input = inputPositions[static_cast<size_t> (inputIndex)];
    input.x = positionSection.getProperty (inputPositionX, 0.0f);
    input.y = positionSection.getProperty (inputPositionY, 0.0f);
    input.z = positionSection.getProperty (inputPositionZ, 0.0f);
}

//==============================================================================
// Matrix Calculation
//==============================================================================

float WFSCalculationEngine::distance3D (const Position& a, const Position& b)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float dz = b.z - a.z;
    return std::sqrt (dx * dx + dy * dy + dz * dz);
}

//==============================================================================
// Muting Check
//==============================================================================

bool WFSCalculationEngine::isRoutingMuted (int inputIndex, int outputIndex) const
{
    auto mutesSection = valueTreeState.getInputMutesSection (inputIndex);
    if (! mutesSection.isValid())
        return false;

    juce::String mutesStr = mutesSection.getProperty (inputMutes).toString();
    juce::StringArray mutesArray;
    mutesArray.addTokens (mutesStr, ",", "");

    if (outputIndex >= 0 && outputIndex < mutesArray.size())
    {
        return mutesArray[outputIndex].getIntValue() != 0;
    }

    return false;
}

//==============================================================================
// Angular Attenuation
//==============================================================================

float WFSCalculationEngine::calculateAngularAttenuation (int /*inputIndex*/, int outputIndex,
                                                          const Position& inputPos,
                                                          const Position& speakerPos) const
{
    // Get output parameters
    auto positionSection = valueTreeState.getOutputPositionSection (outputIndex);

    int orientationDeg = positionSection.getProperty (outputOrientation, 0);
    int pitchDeg = positionSection.getProperty (outputPitch, outputPitchDefault);
    int angleOnDeg = positionSection.getProperty (outputAngleOn, outputAngleOnDefault);
    int angleOffDeg = positionSection.getProperty (outputAngleOff, outputAngleOffDefault);

    // Optimization: if angleOn >= 90°, all inputs are in the "on" zone
    // (hemisphere behind speaker) - skip angular calculation
    if (angleOnDeg >= 90)
        return 1.0f;

    // Convert to radians
    constexpr float degToRad = juce::MathConstants<float>::pi / 180.0f;
    float orientationRad = static_cast<float> (orientationDeg) * degToRad;
    float pitchRad = static_cast<float> (pitchDeg) * degToRad;
    float angleOnRad = static_cast<float> (angleOnDeg) * degToRad;
    float angleOffRad = static_cast<float> (angleOffDeg) * degToRad;

    // Calculate speaker's rear axis direction vector
    // Orientation: 0° = facing audience (toward -Y), positive = clockwise from above
    // Pitch: 0° = horizontal, positive = up, negative = down
    // Rear axis is opposite of front direction
    float cosPitch = std::cos (pitchRad);
    float sinPitch = std::sin (pitchRad);

    // Rear axis direction (opposite of where speaker points)
    // Front direction for orientation=0, pitch=0 is (0, -1, 0)
    // So rear direction is (0, +1, 0)
    // With orientation rotation: rear = (sin(orientation), cos(orientation), 0) for pitch=0
    // With pitch: z component from pitch, xy components scaled by cos(pitch)
    float rearDirX = cosPitch * std::sin (orientationRad);
    float rearDirY = cosPitch * std::cos (orientationRad);
    float rearDirZ = sinPitch;

    // Vector from speaker to input
    float dx = inputPos.x - speakerPos.x;
    float dy = inputPos.y - speakerPos.y;
    float dz = inputPos.z - speakerPos.z;
    float distance = std::sqrt (dx * dx + dy * dy + dz * dz);

    // Avoid division by zero
    if (distance < 0.001f)
        return 1.0f;  // Input at speaker position - full contribution

    // Dot product between rear axis and input direction (normalized)
    float dotProduct = (dx * rearDirX + dy * rearDirY + dz * rearDirZ) / distance;

    // Clamp to valid range for acos
    dotProduct = juce::jlimit (-1.0f, 1.0f, dotProduct);

    // Angle between rear axis and input direction (0 = directly behind, π = directly in front)
    float angle = std::acos (dotProduct);

    // Calculate attenuation based on angle zones
    // angle <= angleOn: full contribution (1.0)
    // angle >= (π - angleOff): muted (0.0)
    // In between: linear interpolation

    if (angle <= angleOnRad)
        return 1.0f;

    float muteAngle = juce::MathConstants<float>::pi - angleOffRad;

    if (angle >= muteAngle)
        return 0.0f;

    // Linear interpolation in transition zone
    float transitionWidth = muteAngle - angleOnRad;
    if (transitionWidth <= 0.0f)
        return 1.0f;  // No transition zone

    float progress = (angle - angleOnRad) / transitionWidth;
    return 1.0f - progress;
}

void WFSCalculationEngine::recalculateMatrix()
{
    // Copy positions under lock
    std::vector<Position> localInputPositions;
    std::vector<Position> localSpeakerPositions;
    std::vector<Position> localListenerPositions;

    {
        const juce::ScopedLock sl (positionLock);
        localInputPositions = inputPositions;
        localSpeakerPositions = speakerPositions;
        localListenerPositions = listenerPositions;
    }

    // Temporary arrays for calculations
    std::vector<float> newDelays (static_cast<size_t> (numInputs * numOutputs));
    std::vector<float> newLevels (static_cast<size_t> (numInputs * numOutputs));
    std::vector<float> newHF (static_cast<size_t> (numInputs * numOutputs));

    // Calculate for each input->output pair
    for (int inIdx = 0; inIdx < numInputs; ++inIdx)
    {
        const Position& inputPos = localInputPositions[static_cast<size_t> (inIdx)];

        // Get input attenuation parameters
        auto inputAttenSection = valueTreeState.getInputAttenuationSection (inIdx);
        float inputAtten = inputAttenSection.getProperty (inputAttenuation, inputAttenuationDefault);
        float inputDistAtten = inputAttenSection.getProperty (inputDistanceAttenuation, inputDistanceAttenuationDefault);

        // Get input height factor (0-100%) for distance calculations
        auto inputPosSection = valueTreeState.getInputPositionSection (inIdx);
        int heightFactorPercent = inputPosSection.getProperty (inputHeightFactor, inputHeightFactorDefault);
        float heightFactor = static_cast<float> (heightFactorPercent) / 100.0f;

        for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
        {
            const size_t matrixIdx = static_cast<size_t> (inIdx * numOutputs + outIdx);

            // ==========================================
            // MUTING CHECK
            // ==========================================
            // Skip calculations if this input→output routing is muted
            if (isRoutingMuted (inIdx, outIdx))
            {
                newDelays[matrixIdx] = 0.0f;
                newLevels[matrixIdx] = 0.0f;
                newHF[matrixIdx] = 0.0f;
                continue;
            }

            const Position& speakerPos = localSpeakerPositions[static_cast<size_t> (outIdx)];
            const Position& listenerPos = localListenerPositions[static_cast<size_t> (outIdx)];

            // ==========================================
            // ANGULAR ATTENUATION
            // ==========================================
            // Check if input is in the "off" zone in front of speaker
            // Note: Uses original inputPos (no height factor applied)
            float angularAtten = calculateAngularAttenuation (inIdx, outIdx, inputPos, speakerPos);

            // If angular attenuation is 0, input is in mute zone - skip further calculations
            if (angularAtten <= 0.0f)
            {
                newDelays[matrixIdx] = 0.0f;
                newLevels[matrixIdx] = 0.0f;
                newHF[matrixIdx] = 0.0f;
                continue;
            }

            // Get output parameters
            auto outputOptionsSection = valueTreeState.getOutputOptionsSection (outIdx);
            auto outputPositionSection = valueTreeState.getOutputPositionSection (outIdx);

            float outputDistAttenPercent = outputOptionsSection.getProperty (outputDistanceAttenPercent, 100.0f);
            float outputHFdamp = outputPositionSection.getProperty (outputHFdamping, outputHFdampingDefault);

            // ==========================================
            // DISTANCE CALCULATIONS (with height factor)
            // ==========================================
            // Apply height factor to Z component for distance calculations only
            // This scales how much the height difference contributes to distance
            auto distanceWithHeightFactor = [heightFactor] (const Position& a, const Position& b) -> float
            {
                float dx = b.x - a.x;
                float dy = b.y - a.y;
                float dz = (b.z - a.z) * heightFactor;  // Scale Z difference
                return std::sqrt (dx * dx + dy * dy + dz * dz);
            };

            float inputToListener = distanceWithHeightFactor (inputPos, listenerPos);
            float inputToSpeaker = distanceWithHeightFactor (inputPos, speakerPos);
            float speakerToListener = distance3D (speakerPos, listenerPos);  // No height factor for speaker→listener

            // ==========================================
            // DELAY CALCULATION
            // ==========================================
            // Delay = (distance input->listener - distance speaker->listener) / speed of sound
            float delayMeters = inputToListener - speakerToListener;
            float delayMs = (delayMeters / speedOfSound) * 1000.0f;
            delayMs = juce::jmax (0.0f, delayMs);  // Clamp to minimum 0

            newDelays[matrixIdx] = delayMs;

            // ==========================================
            // LEVEL CALCULATION
            // ==========================================
            // Level (dB) = inputAttenuation + inputDistanceAttenuation * distance * (outputDistAttenPercent/100)
            float attenuationDb = inputAtten + inputDistAtten * inputToSpeaker * (outputDistAttenPercent / 100.0f);

            // Clamp to reasonable range (-92dB to 0dB)
            attenuationDb = juce::jlimit (-92.0f, 0.0f, attenuationDb);

            // Convert dB to linear
            float linearLevel = std::pow (10.0f, attenuationDb / 20.0f);

            // Apply angular attenuation (linear multiplier 0.0-1.0)
            linearLevel *= angularAtten;

            newLevels[matrixIdx] = linearLevel;

            // ==========================================
            // HF ATTENUATION CALCULATION
            // ==========================================
            // HF attenuation (dB) = outputHFdamping (dB/m) * distance input->speaker
            float hfAtten = outputHFdamp * inputToSpeaker;

            // Clamp to reasonable range
            hfAtten = juce::jlimit (-60.0f, 0.0f, hfAtten);

            newHF[matrixIdx] = hfAtten;
        }
    }

    // Update matrix under lock
    {
        const juce::ScopedLock sl (matrixLock);
        delayTimesMs = std::move (newDelays);
        levels = std::move (newLevels);
        hfAttenuationDb = std::move (newHF);
    }
}

//==============================================================================
// Matrix Access
//==============================================================================

float WFSCalculationEngine::getDelayMs (int inputIndex, int outputIndex) const
{
    if (inputIndex < 0 || inputIndex >= numInputs ||
        outputIndex < 0 || outputIndex >= numOutputs)
        return 0.0f;

    const juce::ScopedLock sl (matrixLock);
    return delayTimesMs[static_cast<size_t> (inputIndex * numOutputs + outputIndex)];
}

float WFSCalculationEngine::getLevel (int inputIndex, int outputIndex) const
{
    if (inputIndex < 0 || inputIndex >= numInputs ||
        outputIndex < 0 || outputIndex >= numOutputs)
        return 0.0f;

    const juce::ScopedLock sl (matrixLock);
    return levels[static_cast<size_t> (inputIndex * numOutputs + outputIndex)];
}

float WFSCalculationEngine::getHFAttenuation (int inputIndex, int outputIndex) const
{
    if (inputIndex < 0 || inputIndex >= numInputs ||
        outputIndex < 0 || outputIndex >= numOutputs)
        return 0.0f;

    const juce::ScopedLock sl (matrixLock);
    return hfAttenuationDb[static_cast<size_t> (inputIndex * numOutputs + outputIndex)];
}

//==============================================================================
// Tree Navigation
//==============================================================================

int WFSCalculationEngine::findOutputIndexFromTree (const juce::ValueTree& tree) const
{
    juce::ValueTree current = tree;

    while (current.isValid())
    {
        if (current.getType() == Output)
        {
            auto parent = current.getParent();
            if (parent.isValid())
                return parent.indexOf (current);
        }
        current = current.getParent();
    }

    return -1;
}

int WFSCalculationEngine::findInputIndexFromTree (const juce::ValueTree& tree) const
{
    juce::ValueTree current = tree;

    while (current.isValid())
    {
        if (current.getType() == Input)
        {
            auto parent = current.getParent();
            if (parent.isValid())
                return parent.indexOf (current);
        }
        current = current.getParent();
    }

    return -1;
}

//==============================================================================
// ValueTree::Listener
//==============================================================================

void WFSCalculationEngine::valueTreePropertyChanged (juce::ValueTree& tree,
                                                      const juce::Identifier& property)
{
    // Output position/parallax properties
    bool isOutputPositionProperty = (property == outputPositionX ||
                                     property == outputPositionY ||
                                     property == outputPositionZ ||
                                     property == outputOrientation);

    bool isOutputParallaxProperty = (property == outputHparallax ||
                                     property == outputVparallax);

    if (isOutputPositionProperty || isOutputParallaxProperty)
    {
        int outputIndex = findOutputIndexFromTree (tree);

        if (outputIndex >= 0 && outputIndex < numOutputs)
        {
            const juce::ScopedLock sl (positionLock);

            if (isOutputPositionProperty)
                updateSpeakerPosition (outputIndex);

            recalculateListenerPosition (outputIndex);
        }
        return;
    }

    // Input position properties
    bool isInputPositionProperty = (property == inputPositionX ||
                                    property == inputPositionY ||
                                    property == inputPositionZ);

    if (isInputPositionProperty)
    {
        int inputIndex = findInputIndexFromTree (tree);

        if (inputIndex >= 0 && inputIndex < numInputs)
        {
            const juce::ScopedLock sl (positionLock);
            updateInputPosition (inputIndex);
        }
    }

    // Note: Matrix recalculation is not triggered automatically here.
    // It should be called at 50Hz from a timer for smooth updates.
}
