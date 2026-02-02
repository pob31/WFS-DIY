#include "WFSCalculationEngine.h"
#include <array>
#include <limits>

using namespace WFSParameterIDs;
using namespace WFSParameterDefaults;

//==============================================================================
WFSCalculationEngine::WFSCalculationEngine (WFSValueTreeState& state)
    : valueTreeState (state)
{
    numInputs = maxInputChannels;
    numOutputs = maxOutputChannels;
    numReverbs = maxReverbChannels;

    // Reserve space for positions
    listenerPositions.resize (static_cast<size_t> (numOutputs));
    speakerPositions.resize (static_cast<size_t> (numOutputs));
    inputPositions.resize (static_cast<size_t> (numInputs));
    speedLimitedPositions.resize (static_cast<size_t> (numInputs));  // Speed-limited interpolated positions
    compositeInputPositions.resize (static_cast<size_t> (numInputs));  // Final positions (speed-limited + flip + offset + LFO)
    reverbFeedPositions.resize (static_cast<size_t> (numReverbs));
    reverbReturnPositions.resize (static_cast<size_t> (numReverbs));
    lfoOffsets.resize (static_cast<size_t> (numInputs));  // LFO position offsets
    gyrophoneOffsets.resize (static_cast<size_t> (numInputs), 0.0f);  // Gyrophone rotation offsets
    previousMinimalLatencyMode.resize (static_cast<size_t> (numInputs), -1);  // -1 = uninitialized
    delayModeRampOffset.resize (static_cast<size_t> (numInputs), 0.0f);  // Start at 0
    previousCommonAttenPercent.resize (static_cast<size_t> (numInputs), -1.0f);  // -1 = uninitialized
    commonAttenRampOffsetDb.resize (static_cast<size_t> (numInputs), 0.0f);  // Start at 0
    commonAttenRampTimeRemaining.resize (static_cast<size_t> (numInputs), 0.0f);  // No active ramps

    // Reserve space for Input → Output matrix results
    const size_t matrixSize = static_cast<size_t> (numInputs * numOutputs);
    delayTimesMs.resize (matrixSize, 0.0f);
    levels.resize (matrixSize, 0.0f);
    hfAttenuationDb.resize (matrixSize, 0.0f);

    // Reserve space for Floor Reflection matrix results
    frDelayTimesMs.resize (matrixSize, 0.0f);
    frLevels.resize (matrixSize, 0.0f);
    frHFAttenuationDb.resize (matrixSize, 0.0f);

    // Reserve space for Input → Reverb Feed matrix results
    const size_t inputReverbSize = static_cast<size_t> (numInputs * numReverbs);
    inputReverbDelayTimesMs.resize (inputReverbSize, 0.0f);
    inputReverbLevels.resize (inputReverbSize, 0.0f);
    inputReverbHFAttenuationDb.resize (inputReverbSize, 0.0f);

    // Reserve space for Reverb Return → Output matrix results
    const size_t reverbOutputSize = static_cast<size_t> (numReverbs * numOutputs);
    reverbOutputDelayTimesMs.resize (reverbOutputSize, 0.0f);
    reverbOutputLevels.resize (reverbOutputSize, 0.0f);
    reverbOutputHFAttenuationDb.resize (reverbOutputSize, 0.0f);

    // Initialize per-input dirty flags (all dirty initially)
    inputDirtyFlags.resize (static_cast<size_t> (numInputs), true);

    // Calculate initial positions
    recalculateAllListenerPositions();
    recalculateAllInputPositions();
    recalculateAllReverbPositions();

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

WFSCalculationEngine::Position WFSCalculationEngine::getCompositeInputPosition (int inputIndex) const
{
    if (inputIndex < 0 || inputIndex >= numInputs)
        return {};

    const juce::ScopedLock sl (positionLock);
    return compositeInputPositions[static_cast<size_t> (inputIndex)];
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

    // Mark outputs as dirty so recalculateMatrix knows to recalculate all input->output pairs
    outputsDirty.store(true);
    matrixDirty.store(true);
}

void WFSCalculationEngine::recalculateAllInputPositions()
{
    const juce::ScopedLock sl (positionLock);

    for (int i = 0; i < numInputs; ++i)
        updateInputPosition (i);

    // Mark all inputs as dirty so recalculateMatrix knows to recalculate all pairs
    for (int i = 0; i < numInputs; ++i)
        inputDirtyFlags[static_cast<size_t>(i)] = true;
    matrixDirty.store(true);
}

void WFSCalculationEngine::markAllInputsDirty()
{
    const juce::ScopedLock sl (positionLock);

    // Mark all inputs as dirty (used when LS gains change to force level recalculation)
    for (int i = 0; i < numInputs; ++i)
        inputDirtyFlags[static_cast<size_t>(i)] = true;
    matrixDirty.store(true);
}

void WFSCalculationEngine::markInputDirty(int inputIndex)
{
    if (inputIndex < 0 || inputIndex >= numInputs)
        return;

    const juce::ScopedLock sl(positionLock);
    inputDirtyFlags[static_cast<size_t>(inputIndex)] = true;
    matrixDirty.store(true);
}

//==============================================================================
// LFO Offset Support
//==============================================================================

void WFSCalculationEngine::setLFOOffset (int inputIndex, float x, float y, float z)
{
    if (inputIndex < 0 || inputIndex >= numInputs)
        return;

    const juce::ScopedLock sl (positionLock);
    auto& offset = lfoOffsets[static_cast<size_t> (inputIndex)];

    // Only mark dirty if offset actually changed (with small tolerance)
    constexpr float epsilon = 0.0001f;
    if (std::abs(offset.x - x) > epsilon ||
        std::abs(offset.y - y) > epsilon ||
        std::abs(offset.z - z) > epsilon)
    {
        offset.x = x;
        offset.y = y;
        offset.z = z;
        // Mark only this specific input as dirty
        inputDirtyFlags[static_cast<size_t> (inputIndex)] = true;
        matrixDirty.store(true);
    }
}

WFSCalculationEngine::Position WFSCalculationEngine::getLFOOffset (int inputIndex) const
{
    if (inputIndex < 0 || inputIndex >= numInputs)
        return {};

    const juce::ScopedLock sl (positionLock);
    return lfoOffsets[static_cast<size_t> (inputIndex)];
}

void WFSCalculationEngine::setGyrophoneOffset (int inputIndex, float offsetRad)
{
    if (inputIndex < 0 || inputIndex >= numInputs)
        return;

    const juce::ScopedLock sl (positionLock);
    auto& offset = gyrophoneOffsets[static_cast<size_t> (inputIndex)];

    // Only mark dirty if offset actually changed (with small tolerance)
    constexpr float epsilon = 0.0001f;
    if (std::abs(offset - offsetRad) > epsilon)
    {
        offset = offsetRad;
        // Mark only this specific input as dirty
        inputDirtyFlags[static_cast<size_t> (inputIndex)] = true;
        matrixDirty.store(true);
    }
}

float WFSCalculationEngine::getGyrophoneOffset (int inputIndex) const
{
    if (inputIndex < 0 || inputIndex >= numInputs)
        return 0.0f;

    const juce::ScopedLock sl (positionLock);
    return gyrophoneOffsets[static_cast<size_t> (inputIndex)];
}

//==============================================================================
// Speed-Limited Position Support
//==============================================================================

void WFSCalculationEngine::setSpeedLimitedPosition (int inputIndex, float x, float y, float z)
{
    if (inputIndex < 0 || inputIndex >= numInputs)
        return;

    const juce::ScopedLock sl (positionLock);
    auto& pos = speedLimitedPositions[static_cast<size_t> (inputIndex)];

    // Only mark dirty if position actually changed (with small tolerance)
    constexpr float epsilon = 0.0001f;
    if (std::abs(pos.x - x) > epsilon ||
        std::abs(pos.y - y) > epsilon ||
        std::abs(pos.z - z) > epsilon)
    {
        pos.x = x;
        pos.y = y;
        pos.z = z;
        // Mark only this specific input as dirty
        inputDirtyFlags[static_cast<size_t> (inputIndex)] = true;
        matrixDirty.store(true);
    }
}

WFSCalculationEngine::Position WFSCalculationEngine::getSpeedLimitedPosition (int inputIndex) const
{
    if (inputIndex < 0 || inputIndex >= numInputs)
        return {};

    const juce::ScopedLock sl (positionLock);
    return speedLimitedPositions[static_cast<size_t> (inputIndex)];
}

//==============================================================================
// Delay Mode Ramp Support
//==============================================================================

void WFSCalculationEngine::updateDelayModeRamps (float deltaTimeSeconds)
{
    const juce::ScopedLock sl (positionLock);

    bool anyChange = false;

    // === Delay mode ramps (fixed 1 second decay) ===
    const float delayRampTimeSeconds = 1.0f;
    const float delayDecayFactor = deltaTimeSeconds / delayRampTimeSeconds;

    for (int i = 0; i < numInputs; ++i)
    {
        auto& offset = delayModeRampOffset[static_cast<size_t> (i)];
        if (std::abs (offset) > 0.001f)  // Only process non-zero offsets
        {
            // Linear decay toward zero
            float decay = offset * delayDecayFactor;
            offset -= decay;

            // Zero out tiny values
            if (std::abs (offset) < 0.001f)
                offset = 0.0f;

            // Keep this input dirty while ramp is active so it gets recalculated
            inputDirtyFlags[static_cast<size_t> (i)] = true;
            anyChange = true;
        }
    }

    // === Common attenuation ramps (variable decay time based on change magnitude) ===
    for (int i = 0; i < numInputs; ++i)
    {
        auto& offsetDb = commonAttenRampOffsetDb[static_cast<size_t> (i)];
        auto& timeRemaining = commonAttenRampTimeRemaining[static_cast<size_t> (i)];

        if (std::abs (offsetDb) > 0.001f && timeRemaining > 0.0f)
        {
            // Decay proportionally: offset reduces to 0 as timeRemaining goes to 0
            // decayFactor = deltaTime / timeRemaining (but capped to avoid overshoot)
            float decayFactor = juce::jmin (1.0f, deltaTimeSeconds / timeRemaining);
            float decay = offsetDb * decayFactor;
            offsetDb -= decay;

            // Update remaining time
            timeRemaining = juce::jmax (0.0f, timeRemaining - deltaTimeSeconds);

            // Zero out tiny values
            if (std::abs (offsetDb) < 0.001f || timeRemaining <= 0.0f)
            {
                offsetDb = 0.0f;
                timeRemaining = 0.0f;
            }

            // Keep this input dirty while ramp is active so it gets recalculated
            inputDirtyFlags[static_cast<size_t> (i)] = true;
            anyChange = true;
        }
    }

    // If any offset changed, mark matrix dirty for recalculation
    if (anyChange)
        matrixDirty.store (true);
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

void WFSCalculationEngine::applyCompositeConstraints (Position& pos, const juce::ValueTree& posSection) const
{
    constexpr float absoluteLimit = 50.0f;  // Maximum ±50m for any axis

    // Get stage bounds from stage state
    auto stageState = valueTreeState.getStageState();
    if (! stageState.isValid())
    {
        // Fallback: just apply absolute limits
        pos.x = juce::jlimit (-absoluteLimit, absoluteLimit, pos.x);
        pos.y = juce::jlimit (-absoluteLimit, absoluteLimit, pos.y);
        pos.z = juce::jlimit (-absoluteLimit, absoluteLimit, pos.z);
        return;
    }

    int currentStageShape = stageState.getProperty (WFSParameterIDs::stageShape, 0);
    float halfWidth = (currentStageShape == 0)
        ? static_cast<float> (stageState.getProperty (WFSParameterIDs::stageWidth, 20.0f)) / 2.0f
        : static_cast<float> (stageState.getProperty (WFSParameterIDs::stageDiameter, 20.0f)) / 2.0f;
    float halfDepth = (currentStageShape == 0)
        ? static_cast<float> (stageState.getProperty (WFSParameterIDs::stageDepth, 20.0f)) / 2.0f
        : static_cast<float> (stageState.getProperty (WFSParameterIDs::stageDiameter, 20.0f)) / 2.0f;
    float stageHeightVal = static_cast<float> (stageState.getProperty (WFSParameterIDs::stageHeight, 10.0f));
    float originW = static_cast<float> (stageState.getProperty (WFSParameterIDs::originWidth, 0.0f));
    float originD = static_cast<float> (stageState.getProperty (WFSParameterIDs::originDepth, 0.0f));
    float originH = static_cast<float> (stageState.getProperty (WFSParameterIDs::originHeight, 0.0f));

    // Calculate stage bounds
    float minX = -halfWidth - originW;
    float maxX = halfWidth - originW;
    float minY = -halfDepth - originD;
    float maxY = halfDepth - originD;
    float minZ = -originH;
    float maxZ = stageHeightVal - originH;

    // Check constraint toggles from input's position section
    bool constraintX = static_cast<int> (posSection.getProperty (inputConstraintX, 1)) != 0;
    bool constraintY = static_cast<int> (posSection.getProperty (inputConstraintY, 1)) != 0;
    bool constraintZ = static_cast<int> (posSection.getProperty (inputConstraintZ, 1)) != 0;

    // Apply X constraint (stage bounds if enabled, always absolute limit)
    if (constraintX)
        pos.x = juce::jlimit (minX, maxX, pos.x);
    pos.x = juce::jlimit (-absoluteLimit, absoluteLimit, pos.x);

    // Apply Y constraint (stage bounds if enabled, always absolute limit)
    if (constraintY)
        pos.y = juce::jlimit (minY, maxY, pos.y);
    pos.y = juce::jlimit (-absoluteLimit, absoluteLimit, pos.y);

    // Apply Z constraint (stage bounds if enabled, always absolute limit)
    if (constraintZ)
        pos.z = juce::jlimit (minZ, maxZ, pos.z);
    pos.z = juce::jlimit (-absoluteLimit, absoluteLimit, pos.z);

    // Apply radius constraint (always enforce 50m max)
    float radius = std::sqrt (pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
    if (radius > absoluteLimit)
    {
        float scale = absoluteLimit / radius;
        pos.x *= scale;
        pos.y *= scale;
        pos.z *= scale;
    }
}

//==============================================================================
// Reverb Position Access and Update
//==============================================================================

WFSCalculationEngine::Position WFSCalculationEngine::getReverbFeedPosition (int reverbIndex) const
{
    if (reverbIndex < 0 || reverbIndex >= numReverbs)
        return {};

    const juce::ScopedLock sl (positionLock);
    return reverbFeedPositions[static_cast<size_t> (reverbIndex)];
}

WFSCalculationEngine::Position WFSCalculationEngine::getReverbReturnPosition (int reverbIndex) const
{
    if (reverbIndex < 0 || reverbIndex >= numReverbs)
        return {};

    const juce::ScopedLock sl (positionLock);
    return reverbReturnPositions[static_cast<size_t> (reverbIndex)];
}

void WFSCalculationEngine::recalculateAllReverbPositions()
{
    const juce::ScopedLock sl (positionLock);

    for (int i = 0; i < numReverbs; ++i)
    {
        updateReverbFeedPosition (i);
        updateReverbReturnPosition (i);
    }

    // Mark reverbs as dirty so recalculateMatrix knows to recalculate all reverb-related pairs
    reverbsDirty.store(true);
    matrixDirty.store(true);
}

void WFSCalculationEngine::updateReverbFeedPosition (int reverbIndex)
{
    // Note: Assumes positionLock is already held

    auto positionSection = valueTreeState.getReverbPositionSection (reverbIndex);

    Position& feedPos = reverbFeedPositions[static_cast<size_t> (reverbIndex)];
    feedPos.x = positionSection.getProperty (reverbPositionX, reverbPositionDefault);
    feedPos.y = positionSection.getProperty (reverbPositionY, reverbPositionDefault);
    feedPos.z = positionSection.getProperty (reverbPositionZ, reverbPositionDefault);
}

void WFSCalculationEngine::updateReverbReturnPosition (int reverbIndex)
{
    // Note: Assumes positionLock is already held
    // Return position = Feed position + offset

    auto positionSection = valueTreeState.getReverbPositionSection (reverbIndex);

    const Position& feedPos = reverbFeedPositions[static_cast<size_t> (reverbIndex)];

    float offsetX = positionSection.getProperty (reverbReturnOffsetX, reverbReturnOffsetDefault);
    float offsetY = positionSection.getProperty (reverbReturnOffsetY, reverbReturnOffsetDefault);
    float offsetZ = positionSection.getProperty (reverbReturnOffsetZ, reverbReturnOffsetDefault);

    Position& returnPos = reverbReturnPositions[static_cast<size_t> (reverbIndex)];
    returnPos.x = feedPos.x + offsetX;
    returnPos.y = feedPos.y + offsetY;
    returnPos.z = feedPos.z + offsetZ;
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

float WFSCalculationEngine::calculateSidelineAttenuation (int inputIndex, const Position& inputPos) const
{
    // Get sidelines parameters from Mutes section
    auto mutesSection = valueTreeState.getInputMutesSection (inputIndex);
    if (! mutesSection.isValid())
        return 1.0f;

    int sidelinesActive = mutesSection.getProperty (inputSidelinesActive, 0);
    if (sidelinesActive == 0)
        return 1.0f;  // Sidelines disabled - no attenuation

    float fringeSize = static_cast<float> (mutesSection.getProperty (inputSidelinesFringe, 1.0f));

    // Get stage parameters
    auto stageState = valueTreeState.getStageState();
    if (! stageState.isValid())
        return 1.0f;

    int currentStageShape = stageState.getProperty (WFSParameterIDs::stageShape, 0);
    float originW = static_cast<float> (stageState.getProperty (originWidth, 0.0f));
    float originD = static_cast<float> (stageState.getProperty (originDepth, -5.0f));

    float distanceFromEdge = 0.0f;

    if (currentStageShape == 0)  // Box stage
    {
        float stageW = static_cast<float> (stageState.getProperty (WFSParameterIDs::stageWidth, 20.0f));
        float stageD = static_cast<float> (stageState.getProperty (WFSParameterIDs::stageDepth, 10.0f));

        // Calculate stage bounds in origin-relative coordinates
        float leftEdge = -stageW / 2.0f - originW;
        float rightEdge = stageW / 2.0f - originW;
        float upstageEdge = stageD / 2.0f - originD;  // Back (positive Y)
        // Downstage edge is NOT included (front toward audience)

        // Calculate minimum distance to any relevant edge (left, right, upstage)
        float distToLeft = inputPos.x - leftEdge;
        float distToRight = rightEdge - inputPos.x;
        float distToUpstage = upstageEdge - inputPos.y;

        distanceFromEdge = juce::jmin (distToLeft, distToRight, distToUpstage);
    }
    else  // Cylinder (1) or Dome (2) - radial distance
    {
        float stageDiam = static_cast<float> (stageState.getProperty (WFSParameterIDs::stageDiameter, 20.0f));
        float stageRadius = stageDiam / 2.0f;

        // Calculate radial distance from stage center
        float dx = inputPos.x + originW;  // Offset to center
        float dy = inputPos.y + originD;
        float radialDistance = std::sqrt (dx * dx + dy * dy);

        // Distance from circular edge
        distanceFromEdge = stageRadius - radialDistance;
    }

    // Apply fringe zone logic
    // fringeSize = total fringe zone
    // Outer half (fringeSize/2 to 0): full mute
    // Inner half (fringeSize to fringeSize/2): linear fade
    float halfFringe = fringeSize / 2.0f;

    if (distanceFromEdge <= 0.0f)
    {
        // Outside stage - full mute
        return 0.0f;
    }
    else if (distanceFromEdge <= halfFringe)
    {
        // In outer half of fringe - full mute
        return 0.0f;
    }
    else if (distanceFromEdge <= fringeSize)
    {
        // In inner half of fringe - linear fade
        // At halfFringe: attenuation = 0
        // At fringeSize: attenuation = 1
        float fadeProgress = (distanceFromEdge - halfFringe) / halfFringe;
        return fadeProgress;
    }

    // Inside safe zone - no attenuation
    return 1.0f;
}

bool WFSCalculationEngine::isInputReverbMuted (int inputIndex) const
{
    // Check if inputMuteReverbSends is enabled for this input
    auto mutesSection = valueTreeState.getInputMutesSection (inputIndex);
    if (! mutesSection.isValid())
        return false;

    int muteReverbSends = mutesSection.getProperty (inputMuteReverbSends, 0);
    return muteReverbSends != 0;
}

bool WFSCalculationEngine::isReverbOutputMuted (int reverbIndex, int outputIndex) const
{
    // Check reverbMutes array for this reverb->output pair
    auto returnSection = valueTreeState.getReverbReturnSection (reverbIndex);
    if (! returnSection.isValid())
        return false;

    juce::String mutesStr = returnSection.getProperty (reverbMutes).toString();
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
    // Front direction for orientation=0, pitch=0 is (0, -1, 0) pointing toward audience
    // So rear direction is (0, +1, 0)
    // With orientation rotation: positive angles point right (clockwise from above)
    // rear = (-sin(orientation), cos(orientation), 0) for pitch=0
    // With pitch: z component from pitch, xy components scaled by cos(pitch)
    float rearDirX = -cosPitch * std::sin (orientationRad);
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

float WFSCalculationEngine::calculateReverbFeedAngularAttenuation (int /*inputIndex*/, int reverbIndex,
                                                                    const Position& inputPos,
                                                                    const Position& reverbFeedPos) const
{
    // Get reverb feed parameters (similar to output but from reverb feed section)
    auto feedSection = valueTreeState.getReverbFeedSection (reverbIndex);

    int orientationDeg = feedSection.getProperty (reverbOrientation, reverbOrientationDefault);
    int pitchDeg = feedSection.getProperty (reverbPitch, reverbPitchDefault);
    int angleOnDeg = feedSection.getProperty (reverbAngleOn, reverbAngleOnDefault);
    int angleOffDeg = feedSection.getProperty (reverbAngleOff, reverbAngleOffDefault);

    // Optimization: if angleOn >= 90°, all inputs are in the "on" zone
    if (angleOnDeg >= 90)
        return 1.0f;

    // Convert to radians
    constexpr float degToRad = juce::MathConstants<float>::pi / 180.0f;
    float orientationRad = static_cast<float> (orientationDeg) * degToRad;
    float pitchRad = static_cast<float> (pitchDeg) * degToRad;
    float angleOnRad = static_cast<float> (angleOnDeg) * degToRad;
    float angleOffRad = static_cast<float> (angleOffDeg) * degToRad;

    // Calculate rear axis direction vector (same logic as output angular attenuation)
    // Positive orientation angles point right (clockwise from above)
    float cosPitch = std::cos (pitchRad);
    float sinPitch = std::sin (pitchRad);

    float rearDirX = -cosPitch * std::sin (orientationRad);
    float rearDirY = cosPitch * std::cos (orientationRad);
    float rearDirZ = sinPitch;

    // Vector from reverb feed to input
    float dx = inputPos.x - reverbFeedPos.x;
    float dy = inputPos.y - reverbFeedPos.y;
    float dz = inputPos.z - reverbFeedPos.z;
    float distance = std::sqrt (dx * dx + dy * dy + dz * dz);

    if (distance < 0.001f)
        return 1.0f;

    // Dot product between rear axis and input direction
    float dotProduct = (dx * rearDirX + dy * rearDirY + dz * rearDirZ) / distance;
    dotProduct = juce::jlimit (-1.0f, 1.0f, dotProduct);

    float angle = std::acos (dotProduct);

    if (angle <= angleOnRad)
        return 1.0f;

    float muteAngle = juce::MathConstants<float>::pi - angleOffRad;

    if (angle >= muteAngle)
        return 0.0f;

    float transitionWidth = muteAngle - angleOnRad;
    if (transitionWidth <= 0.0f)
        return 1.0f;

    float progress = (angle - angleOnRad) / transitionWidth;
    return 1.0f - progress;
}

bool WFSCalculationEngine::recalculateMatrixIfDirty()
{
    if (!matrixDirty.load())
        return false;

    recalculateMatrix();
    return true;
}

void WFSCalculationEngine::recalculateMatrix()
{
    // Clear dirty flag at start (any new changes during calc will set it again)
    matrixDirty.store(false);

    // Copy positions under lock and determine which inputs need recalculation
    std::vector<Position> localInputPositions;
    std::vector<Position> localSpeakerPositions;
    std::vector<Position> localListenerPositions;
    std::vector<Position> localReverbFeedPositions;
    std::vector<Position> localReverbReturnPositions;
    std::vector<float> localGyrophoneOffsets;  // Gyrophone rotation offsets per input
    std::vector<bool> inputsToRecalc;

    // Delay mode ramp state (for smooth transitions when toggling inputMinimalLatency)
    std::vector<int> localPreviousMode;
    std::vector<float> localRampOffset;

    // Common attenuation ramp state (for smooth transitions when changing inputCommonAtten)
    std::vector<float> localPrevCommonAttenPercent;
    std::vector<float> localCommonAttenRampOffsetDb;
    std::vector<float> localCommonAttenRampTimeRemaining;

    // Capture dirty state and clear flags
    bool needOutputRecalc = outputsDirty.exchange(false);
    bool needReverbRecalc = reverbsDirty.exchange(false);

    {
        const juce::ScopedLock sl (positionLock);
        localInputPositions = speedLimitedPositions;  // Use speed-limited positions as base
        localSpeakerPositions = speakerPositions;
        localListenerPositions = listenerPositions;
        localReverbFeedPositions = reverbFeedPositions;
        localReverbReturnPositions = reverbReturnPositions;

        // Apply flip transformation and regular offset (before LFO)
        for (size_t i = 0; i < localInputPositions.size(); ++i)
        {
            auto posSection = valueTreeState.getInputPositionSection (static_cast<int> (i));

            // Apply flip (mirror around origin)
            if (static_cast<int> (posSection.getProperty (inputFlipX, 0)) != 0)
                localInputPositions[i].x = -localInputPositions[i].x;
            if (static_cast<int> (posSection.getProperty (inputFlipY, 0)) != 0)
                localInputPositions[i].y = -localInputPositions[i].y;
            if (static_cast<int> (posSection.getProperty (inputFlipZ, 0)) != 0)
                localInputPositions[i].z = -localInputPositions[i].z;

            // Apply regular offset (inputOffsetX/Y/Z)
            localInputPositions[i].x += static_cast<float> (posSection.getProperty (inputOffsetX, 0.0f));
            localInputPositions[i].y += static_cast<float> (posSection.getProperty (inputOffsetY, 0.0f));
            localInputPositions[i].z += static_cast<float> (posSection.getProperty (inputOffsetZ, 0.0f));
        }

        // Apply LFO offsets to input positions
        for (size_t i = 0; i < localInputPositions.size() && i < lfoOffsets.size(); ++i)
        {
            localInputPositions[i].x += lfoOffsets[i].x;
            localInputPositions[i].y += lfoOffsets[i].y;
            localInputPositions[i].z += lfoOffsets[i].z;
        }

        // Apply composite position constraints (safety layer)
        // This clamps the final position without affecting raw position or LFO amplitude
        for (size_t i = 0; i < localInputPositions.size(); ++i)
        {
            auto posSection = valueTreeState.getInputPositionSection (static_cast<int> (i));
            applyCompositeConstraints (localInputPositions[i], posSection);
        }

        // Store composite positions for external access (used by LiveSourceTamerEngine)
        compositeInputPositions = localInputPositions;

        // Copy gyrophone rotation offsets
        localGyrophoneOffsets = gyrophoneOffsets;

        // Copy delay mode ramp state
        localPreviousMode = previousMinimalLatencyMode;
        localRampOffset = delayModeRampOffset;

        // Copy common attenuation ramp state
        localPrevCommonAttenPercent = previousCommonAttenPercent;
        localCommonAttenRampOffsetDb = commonAttenRampOffsetDb;
        localCommonAttenRampTimeRemaining = commonAttenRampTimeRemaining;

        // Capture and clear per-input dirty flags
        // If outputs or reverbs changed, ALL inputs need recalculation
        inputsToRecalc.resize(static_cast<size_t>(numInputs));
        for (int i = 0; i < numInputs; ++i)
        {
            inputsToRecalc[static_cast<size_t>(i)] = needOutputRecalc || needReverbRecalc || inputDirtyFlags[static_cast<size_t>(i)];
            inputDirtyFlags[static_cast<size_t>(i)] = false;
        }
    }

    // Get global config parameters
    auto masterState = valueTreeState.getMasterState();
    float globalHaasEffect = masterState.getProperty (haasEffect, haasEffectDefault);
    float globalSystemLatency = masterState.getProperty (systemLatency, systemLatencyDefault);

    // Temporary arrays for Input → Output calculations
    // Initialize with copies of existing arrays (non-dirty inputs keep their values)
    std::vector<float> newDelays;
    std::vector<float> newLevels;
    std::vector<float> newHF;
    {
        const juce::ScopedLock sl (matrixLock);
        newDelays = delayTimesMs;
        newLevels = levels;
        newHF = hfAttenuationDb;
    }

    // Temporary arrays for Floor Reflection calculations
    std::vector<float> newFRDelays;
    std::vector<float> newFRLevels;
    std::vector<float> newFRHF;
    {
        const juce::ScopedLock sl (matrixLock);
        newFRDelays = frDelayTimesMs;
        newFRLevels = frLevels;
        newFRHF = frHFAttenuationDb;
    }

    // Temporary arrays for Input → Reverb Feed calculations
    std::vector<float> newInputReverbDelays;
    std::vector<float> newInputReverbLevels;
    std::vector<float> newInputReverbHF;
    {
        const juce::ScopedLock sl (matrixLock);
        newInputReverbDelays = inputReverbDelayTimesMs;
        newInputReverbLevels = inputReverbLevels;
        newInputReverbHF = inputReverbHFAttenuationDb;
    }

    // Temporary arrays for Reverb Return → Output calculations
    // (only recalculated if outputs or reverbs changed)
    std::vector<float> newReverbOutputDelays;
    std::vector<float> newReverbOutputLevels;
    std::vector<float> newReverbOutputHF;
    {
        const juce::ScopedLock sl (matrixLock);
        newReverbOutputDelays = reverbOutputDelayTimesMs;
        newReverbOutputLevels = reverbOutputLevels;
        newReverbOutputHF = reverbOutputHFAttenuationDb;
    }

    // Store common attenuation adjustment per input (for reverb feed calculations)
    std::vector<float> inputCommonAttenAdjustments (static_cast<size_t> (numInputs), 0.0f);

    // Temporary arrays for level post-processing (per output, reset per input)
    std::vector<float> tempAttenuationDb (static_cast<size_t> (numOutputs));
    std::vector<float> tempAngularAtten (static_cast<size_t> (numOutputs));

    // Track which outputs are valid for minimal latency calculation (per input)
    // An output is valid if: NOT muted, NOT angleOff'd (level > 0), AND outputMiniLatencyEnable = 1
    std::vector<bool> validForMinLatency (static_cast<size_t> (numOutputs), false);

    // Track which outputs are valid for common attenuation calculation (per input)
    // An output is valid if: NOT muted, NOT angleOff'd
    std::vector<bool> validForCommonAtten (static_cast<size_t> (numOutputs), false);

    // Cache output array assignments (0 = Single, 1-10 = Array 1-10)
    std::vector<int> outputArrayAssignments (static_cast<size_t> (numOutputs), 0);
    for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
    {
        auto outputChannelSection = valueTreeState.getOutputChannelSection (outIdx);
        outputArrayAssignments[static_cast<size_t> (outIdx)] =
            outputChannelSection.getProperty (outputArray, outputArrayDefault);
    }

    // Calculate for each input->output pair
    for (int inIdx = 0; inIdx < numInputs; ++inIdx)
    {
        // Skip inputs that don't need recalculation (existing values preserved from copy)
        if (!inputsToRecalc[static_cast<size_t>(inIdx)])
            continue;

        const Position& inputPos = localInputPositions[static_cast<size_t> (inIdx)];

        // Get input attenuation parameters
        auto inputAttenSection = valueTreeState.getInputAttenuationSection (inIdx);
        float inputAtten = inputAttenSection.getProperty (inputAttenuation, inputAttenuationDefault);
        float inputDistAtten = inputAttenSection.getProperty (inputDistanceAttenuation, inputDistanceAttenuationDefault);
        int attenLaw = inputAttenSection.getProperty (inputAttenuationLaw, inputAttenuationLawDefault);
        float distRatio = inputAttenSection.getProperty (inputDistanceRatio, inputDistanceRatioDefault);
        int commonAttenPercent = inputAttenSection.getProperty (inputCommonAtten, inputCommonAttenDefault);
        float commonAttenFactor = static_cast<float> (commonAttenPercent) / 100.0f;

        // Get per-array attenuation values from the Mutes section (dB, 0 = no attenuation)
        auto inputMutesSection = valueTreeState.getInputMutesSection (inIdx);
        std::array<float, 10> arrayAttenDb = {0.0f};
        arrayAttenDb[0] = inputMutesSection.getProperty (inputArrayAtten1, inputArrayAttenDefault);
        arrayAttenDb[1] = inputMutesSection.getProperty (inputArrayAtten2, inputArrayAttenDefault);
        arrayAttenDb[2] = inputMutesSection.getProperty (inputArrayAtten3, inputArrayAttenDefault);
        arrayAttenDb[3] = inputMutesSection.getProperty (inputArrayAtten4, inputArrayAttenDefault);
        arrayAttenDb[4] = inputMutesSection.getProperty (inputArrayAtten5, inputArrayAttenDefault);
        arrayAttenDb[5] = inputMutesSection.getProperty (inputArrayAtten6, inputArrayAttenDefault);
        arrayAttenDb[6] = inputMutesSection.getProperty (inputArrayAtten7, inputArrayAttenDefault);
        arrayAttenDb[7] = inputMutesSection.getProperty (inputArrayAtten8, inputArrayAttenDefault);
        arrayAttenDb[8] = inputMutesSection.getProperty (inputArrayAtten9, inputArrayAttenDefault);
        arrayAttenDb[9] = inputMutesSection.getProperty (inputArrayAtten10, inputArrayAttenDefault);

        // Get input channel parameters
        auto inputChannelSection = valueTreeState.getInputChannelSection (inIdx);
        int minimalLatencyMode = inputChannelSection.getProperty (inputMinimalLatency, 0);
        float inputDelayLat = inputChannelSection.getProperty (inputDelayLatency, 0.0f);

        // Get input height factor (0-100%) for distance calculations
        auto inputPosSection = valueTreeState.getInputPositionSection (inIdx);
        int heightFactorPercent = inputPosSection.getProperty (inputHeightFactor, inputHeightFactorDefault);
        float heightFactor = static_cast<float> (heightFactorPercent) / 100.0f;

        // Get input directivity parameters
        auto inputDirectivitySection = valueTreeState.getInputDirectivitySection (inIdx);
        int directivityDeg = inputDirectivitySection.getProperty (inputDirectivity, inputDirectivityDefault);
        int rotationDeg = inputDirectivitySection.getProperty (inputRotation, inputRotationDefault);
        int tiltDeg = inputDirectivitySection.getProperty (inputTilt, inputTiltDefault);
        float hfShelfDb = inputDirectivitySection.getProperty (inputHFshelf, inputHFshelfDefault);

        // Convert directivity parameters to radians
        float directivityRad = static_cast<float> (directivityDeg) * (juce::MathConstants<float>::pi / 180.0f);
        float rotationRad = static_cast<float> (rotationDeg) * (juce::MathConstants<float>::pi / 180.0f);
        float tiltRad = static_cast<float> (tiltDeg) * (juce::MathConstants<float>::pi / 180.0f);

        // Add gyrophone rotation offset (Leslie speaker effect for HF directivity)
        if (static_cast<size_t>(inIdx) < localGyrophoneOffsets.size())
            rotationRad += localGyrophoneOffsets[static_cast<size_t>(inIdx)];

        // Precompute facing direction for this input
        // rotation=0 faces toward -Y (audience), tilt=0 is horizontal
        float facingX = std::sin (rotationRad) * std::cos (tiltRad);
        float facingY = -std::cos (rotationRad) * std::cos (tiltRad);
        float facingZ = std::sin (tiltRad);

        // Reset tracking arrays for this input
        std::fill (validForMinLatency.begin(), validForMinLatency.end(), false);
        std::fill (validForCommonAtten.begin(), validForCommonAtten.end(), false);
        std::fill (tempAttenuationDb.begin(), tempAttenuationDb.end(), -92.0f);
        std::fill (tempAngularAtten.begin(), tempAngularAtten.end(), 0.0f);

        for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
        {
            const size_t matrixIdx = static_cast<size_t> (inIdx * numOutputs + outIdx);

            // Get output parameters early (needed for outputMiniLatencyEnable check)
            auto outputOptionsSection = valueTreeState.getOutputOptionsSection (outIdx);
            auto outputPositionSection = valueTreeState.getOutputPositionSection (outIdx);

            int outputMiniLatEnable = outputOptionsSection.getProperty (outputMiniLatencyEnable, 1);

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

            // This output is valid for minimal latency search if outputMiniLatencyEnable = 1
            // (and we've passed mute and angleOff checks)
            if (outputMiniLatEnable == 1)
                validForMinLatency[static_cast<size_t> (outIdx)] = true;

            // This output is valid for common attenuation search
            // (we've passed mute and angleOff checks)
            validForCommonAtten[static_cast<size_t> (outIdx)] = true;

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
            // DELAY CALCULATION (raw distance-based delay)
            // ==========================================
            // Delay = (distance input->listener - distance speaker->listener) / speed of sound
            float delayMeters = inputToListener - speakerToListener;
            float delayMs = (delayMeters / speedOfSound) * 1000.0f;
            delayMs = juce::jmax (0.0f, delayMs);  // Clamp to minimum 0

            // Store raw delay (will be post-processed after inner loop)
            newDelays[matrixIdx] = delayMs;

            // ==========================================
            // LEVEL CALCULATION (store dB for post-processing)
            // ==========================================
            float distanceAttenDb = 0.0f;

            if (attenLaw == 0)
            {
                // Law 0: Linear (dB/m)
                // distanceAttenDb = inputDistanceAttenuation * distance
                distanceAttenDb = inputDistAtten * inputToSpeaker;
            }
            else
            {
                // Law 1: Inverse square (1/d)
                // -6dB per doubling of distance, 0dB at reference distance (distRatio)
                // distanceAttenDb = -20 * log10(distance / distRatio)
                // Clamp inside reference sphere to 0dB (no amplification)
                float effectiveDistance = inputToSpeaker / juce::jmax (0.001f, distRatio);
                if (effectiveDistance < 1.0f)
                    distanceAttenDb = 0.0f;  // Inside reference sphere - no attenuation
                else
                    distanceAttenDb = -20.0f * std::log10 (effectiveDistance);
            }

            // Apply outputDistAttenPercent scaling to the distance-dependent part
            float attenuationDb = inputAtten + distanceAttenDb * (outputDistAttenPercent / 100.0f);

            // Clamp to reasonable range (-92dB to 0dB)
            attenuationDb = juce::jlimit (-92.0f, 0.0f, attenuationDb);

            // Store dB and angular values for post-processing
            // (linear conversion and angular multiplication will be done after common attenuation)
            tempAttenuationDb[static_cast<size_t> (outIdx)] = attenuationDb;
            tempAngularAtten[static_cast<size_t> (outIdx)] = angularAtten;

            // Temporarily mark as active (will be replaced in post-processing)
            newLevels[matrixIdx] = 1.0f;

            // ==========================================
            // HF ATTENUATION CALCULATION
            // ==========================================
            // Two cumulative HF attenuations:
            // 1. Output-based: outputHFdamping (dB/m) * distance
            // 2. Input directivity-based: smooth falloff from inputHFshelf

            // 1. Output-based HF damping
            float hfAttenOutput = outputHFdamp * inputToSpeaker;

            // 2. Input directivity HF attenuation
            float hfAttenDirectivity = 0.0f;

            // Only calculate if directivity < 360° and inputHFshelf < 0dB
            if (directivityDeg < 360 && hfShelfDb < 0.0f)
            {
                // Calculate raw distance (without height factor) for directivity
                float dx = speakerPos.x - inputPos.x;
                float dy = speakerPos.y - inputPos.y;
                float dz = speakerPos.z - inputPos.z;
                float rawDistance = std::sqrt (dx * dx + dy * dy + dz * dz);

                if (rawDistance > 0.001f)
                {
                    // Normalize direction to speaker
                    float invDist = 1.0f / rawDistance;
                    float toSpeakerX = dx * invDist;
                    float toSpeakerY = dy * invDist;
                    float toSpeakerZ = dz * invDist;

                    // Dot product: facing · toSpeaker
                    float dotProduct = facingX * toSpeakerX + facingY * toSpeakerY + facingZ * toSpeakerZ;

                    // Clamp for acos
                    dotProduct = juce::jlimit (-1.0f, 1.0f, dotProduct);

                    // Angle between facing direction and direction to speaker
                    float angleToSpeaker = std::acos (dotProduct);

                    // Half of the brightness cone (directivity defines full cone)
                    float halfDirectivity = directivityRad * 0.5f;

                    // If outside the brightness cone, apply smooth falloff
                    if (angleToSpeaker > halfDirectivity)
                    {
                        // Transition from edge of cone to rear (π)
                        float transitionRange = juce::MathConstants<float>::pi - halfDirectivity;

                        if (transitionRange > 0.001f)
                        {
                            // Progress from 0 (at cone edge) to 1 (at rear)
                            float progress = (angleToSpeaker - halfDirectivity) / transitionRange;
                            progress = juce::jmin (1.0f, progress);

                            // Smooth falloff using sqrt(sin) as in Max patch
                            // At progress=0: sin(0)=0, sqrt=0, attenuation=0
                            // At progress=1: sin(π/2)=1, sqrt=1, attenuation=hfShelfDb
                            float sinArg = progress * juce::MathConstants<float>::halfPi;
                            hfAttenDirectivity = hfShelfDb * std::sqrt (std::sin (sinArg));
                        }
                    }
                    // Inside brightness cone: hfAttenDirectivity stays 0
                }
                // If distance is 0, angle is undefined - no directivity attenuation
            }

            // Combine both HF attenuations (additive in dB)
            float hfAtten = hfAttenOutput + hfAttenDirectivity;

            // Clamp to reasonable range
            hfAtten = juce::jlimit (-60.0f, 0.0f, hfAtten);

            newHF[matrixIdx] = hfAtten;
        }

        // ==========================================
        // DELAY POST-PROCESSING (per input)
        // ==========================================

        // Always calculate minDelay (needed for mode change compensation)
        float minDelay = std::numeric_limits<float>::max();
        bool foundValidOutput = false;

        for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
        {
            if (validForMinLatency[static_cast<size_t> (outIdx)])
            {
                const size_t matrixIdx = static_cast<size_t> (inIdx * numOutputs + outIdx);
                if (newDelays[matrixIdx] < minDelay)
                {
                    minDelay = newDelays[matrixIdx];
                    foundValidOutput = true;
                }
            }
        }

        // Calculate base offset for mode 0 (haas + latency adjustments)
        // Using average output delay latency = 0 as approximation for mode calculation
        float mode0BaseOffset = globalHaasEffect - globalSystemLatency + inputDelayLat;
        float mode1BaseOffset = foundValidOutput ? -minDelay : 0.0f;

        // Detect mode change and calculate compensation offset
        int prevMode = localPreviousMode[static_cast<size_t> (inIdx)];
        if (prevMode != -1 && prevMode != minimalLatencyMode)
        {
            // Mode changed - calculate compensation offset to maintain delay continuity
            // The ramp offset compensates for the instantaneous delay change
            if (prevMode == 0)
            {
                // Switching from Acoustic Precedence (mode 0) to Minimal Latency (mode 1)
                // Old delays had mode0BaseOffset added, new delays have minDelay subtracted
                // Compensation = mode0BaseOffset - mode1BaseOffset
                localRampOffset[static_cast<size_t> (inIdx)] += (mode0BaseOffset - mode1BaseOffset);
            }
            else
            {
                // Switching from Minimal Latency (mode 1) to Acoustic Precedence (mode 0)
                // Old delays had minDelay subtracted, new delays have mode0BaseOffset added
                // Compensation = mode1BaseOffset - mode0BaseOffset
                localRampOffset[static_cast<size_t> (inIdx)] += (mode1BaseOffset - mode0BaseOffset);
            }
        }

        // Update previous mode
        localPreviousMode[static_cast<size_t> (inIdx)] = minimalLatencyMode;

        // Apply mode-specific delay processing
        if (minimalLatencyMode == 0)
        {
            // Mode 0: Acoustic Precedence
            // finalDelay = calculatedDelay + haasEffect - systemLatency + inputDelayLatency + outputDelayLatency
            for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
            {
                const size_t matrixIdx = static_cast<size_t> (inIdx * numOutputs + outIdx);

                // Skip if muted (level = 0)
                if (newLevels[matrixIdx] <= 0.0f)
                    continue;

                // Get output delay latency
                auto outputChannelSection = valueTreeState.getOutputChannelSection (outIdx);
                float outputDelayLat = outputChannelSection.getProperty (outputDelayLatency, 0.0f);

                float finalDelay = newDelays[matrixIdx] + globalHaasEffect - globalSystemLatency
                                   + inputDelayLat + outputDelayLat;

                newDelays[matrixIdx] = juce::jmax (0.0f, finalDelay);
            }
        }
        else
        {
            // Mode 1: Minimal Latency
            // Subtract minimum from all delays (if we found valid outputs)
            if (foundValidOutput)
            {
                for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
                {
                    const size_t matrixIdx = static_cast<size_t> (inIdx * numOutputs + outIdx);

                    // Skip if muted (level = 0)
                    if (newLevels[matrixIdx] <= 0.0f)
                        continue;

                    float finalDelay = newDelays[matrixIdx] - minDelay;
                    newDelays[matrixIdx] = juce::jmax (0.0f, finalDelay);
                }
            }
            // If no valid outputs found, leave delays as calculated (no subtraction)
        }

        // Apply ramp offset for smooth mode transitions
        float rampOffset = localRampOffset[static_cast<size_t> (inIdx)];
        if (std::abs (rampOffset) > 0.001f)
        {
            for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
            {
                const size_t matrixIdx = static_cast<size_t> (inIdx * numOutputs + outIdx);
                if (newLevels[matrixIdx] > 0.0f)
                {
                    newDelays[matrixIdx] = juce::jmax (0.0f, newDelays[matrixIdx] + rampOffset);
                }
            }
        }

        // ==========================================
        // LEVEL POST-PROCESSING - Common Attenuation (per input)
        // ==========================================
        // Purpose: Prevent upstage sources from losing too much overall level
        // by raising all attenuations toward the minimum (closest to 0dB)

        float commonAttenAdjustment = 0.0f;
        float minAttenuation = -92.0f;  // Track this for ramp calculation
        bool foundValidForCommon = false;

        // Note: commonAttenFactor interpretation:
        // 100% = keep full original attenuation (no lift applied)
        // 0% = apply full lift (all outputs raised to match minimum attenuation)

        // Find minimum attenuation (needed for both current calc and ramp compensation)
        for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
        {
            if (validForCommonAtten[static_cast<size_t> (outIdx)])
            {
                float atten = tempAttenuationDb[static_cast<size_t> (outIdx)];
                if (atten > minAttenuation)
                {
                    minAttenuation = atten;
                    foundValidForCommon = true;
                }
            }
        }

        // Calculate current adjustment
        if (commonAttenFactor < 1.0f && foundValidForCommon)
        {
            // minAttenuation is negative (e.g., -20dB), so -minAttenuation is positive (e.g., +20dB)
            // Invert factor: 0% → full lift, 100% → no lift
            commonAttenAdjustment = -minAttenuation * (1.0f - commonAttenFactor);
        }

        // Detect common attenuation percentage change and set up ramp
        float prevPercent = localPrevCommonAttenPercent[static_cast<size_t> (inIdx)];
        float currentPercent = static_cast<float> (commonAttenPercent);

        if (prevPercent >= 0.0f && std::abs (prevPercent - currentPercent) > 0.1f)
        {
            // Calculate what adjustment WOULD have been with old percentage
            float oldFactor = prevPercent / 100.0f;
            float oldAdjustment = 0.0f;
            if (oldFactor < 1.0f && foundValidForCommon)
            {
                oldAdjustment = -minAttenuation * (1.0f - oldFactor);
            }

            // Compensation offset = old adjustment - new adjustment
            // This keeps the level continuous at the moment of change
            float compensation = oldAdjustment - commonAttenAdjustment;

            // Add to existing ramp offset (in case of rapid changes)
            localCommonAttenRampOffsetDb[static_cast<size_t> (inIdx)] += compensation;

            // Set ramp time proportional to change magnitude
            // 1% change = 0.01s, 50% change = 0.5s, 100% change = 1.0s
            float deltaPercent = std::abs (currentPercent - prevPercent);
            float rampTime = deltaPercent * 0.01f;  // 0.01 seconds per percentage point
            localCommonAttenRampTimeRemaining[static_cast<size_t> (inIdx)] = rampTime;
        }

        // Update previous percentage for next calculation
        localPrevCommonAttenPercent[static_cast<size_t> (inIdx)] = currentPercent;

        // Store the common attenuation adjustment for later use in reverb feed calculations
        inputCommonAttenAdjustments[static_cast<size_t> (inIdx)] = commonAttenAdjustment;

        // Get current ramp offset for this input
        float commonAttenRampOffset = localCommonAttenRampOffsetDb[static_cast<size_t> (inIdx)];

        // Apply common attenuation adjustment and convert to linear levels
        for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
        {
            const size_t matrixIdx = static_cast<size_t> (inIdx * numOutputs + outIdx);

            // Skip if muted (newLevels is already 0)
            if (newLevels[matrixIdx] <= 0.0f)
                continue;

            // Get stored values
            float attenuationDb = tempAttenuationDb[static_cast<size_t> (outIdx)];
            float angularAtten = tempAngularAtten[static_cast<size_t> (outIdx)];

            // Apply common attenuation adjustment + ramp offset
            attenuationDb += commonAttenAdjustment + commonAttenRampOffset;

            // Apply per-array attenuation (if output is assigned to an array 1-10)
            int outputArrayNum = outputArrayAssignments[static_cast<size_t> (outIdx)];
            if (outputArrayNum >= 1 && outputArrayNum <= 10)
                attenuationDb += arrayAttenDb[static_cast<size_t> (outputArrayNum - 1)];

            // Clamp to 0dB max (don't amplify) and -92dB min
            attenuationDb = juce::jlimit (-92.0f, 0.0f, attenuationDb);

            // Convert dB to linear
            float linearLevel = std::pow (10.0f, attenuationDb / 20.0f);

            // Apply angular attenuation (linear multiplier 0.0-1.0)
            linearLevel *= angularAtten;

            // Apply Live Source Tamer gain (linear multiplier 0.0-1.0)
            if (sharedLSGains != nullptr)
                linearLevel *= sharedLSGains[matrixIdx];

            // Apply Sideline attenuation (linear multiplier 0.0-1.0)
            float sidelineAtten = calculateSidelineAttenuation (inIdx, inputPos);
            linearLevel *= sidelineAtten;

            newLevels[matrixIdx] = linearLevel;
        }
    }

    // ==========================================================================
    // FLOOR REFLECTION CALCULATIONS
    // ==========================================================================
    // Floor reflections simulate sound bouncing off the floor (z=0 plane).
    // For each input/output pair, calculate extra delay and attenuation for
    // the reflected path through position (x, y, -z).

    for (int inIdx = 0; inIdx < numInputs; ++inIdx)
    {
        // Skip inputs that don't need recalculation
        if (!inputsToRecalc[static_cast<size_t>(inIdx)])
            continue;

        const Position& inputPos = localInputPositions[static_cast<size_t>(inIdx)];

        // Get FR parameters from Hackoustics section
        auto hackousticsSection = valueTreeState.getInputHackousticsSection(inIdx);
        int frActive = hackousticsSection.getProperty(inputFRactive, 0);

        // If FR not active for this input, zero all FR entries
        if (frActive == 0)
        {
            for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
            {
                const size_t matrixIdx = static_cast<size_t>(inIdx * numOutputs + outIdx);
                newFRDelays[matrixIdx] = 0.0f;
                newFRLevels[matrixIdx] = 0.0f;
                newFRHF[matrixIdx] = 0.0f;
            }
            continue;
        }

        // Skip if source is at or below floor (z <= 0) - no reflection possible
        if (inputPos.z <= 0.0f)
        {
            for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
            {
                const size_t matrixIdx = static_cast<size_t>(inIdx * numOutputs + outIdx);
                newFRDelays[matrixIdx] = 0.0f;
                newFRLevels[matrixIdx] = 0.0f;
                newFRHF[matrixIdx] = 0.0f;
            }
            continue;
        }

        // Calculate reflected position (mirror across z=0 plane)
        Position reflectedPos = { inputPos.x, inputPos.y, -inputPos.z };

        // Get FR parameters
        float frAttenDb = hackousticsSection.getProperty(inputFRattenuation, -3.0f);

        for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
        {
            const size_t matrixIdx = static_cast<size_t>(inIdx * numOutputs + outIdx);

            // Get output FR enable parameter
            auto outputOptionsSection = valueTreeState.getOutputOptionsSection(outIdx);
            int outputFRen = outputOptionsSection.getProperty(outputFRenable, 1);

            // Skip if output FR disabled OR direct signal is muted (level = 0)
            if (outputFRen == 0 || newLevels[matrixIdx] <= 0.0f)
            {
                newFRDelays[matrixIdx] = 0.0f;
                newFRLevels[matrixIdx] = 0.0f;
                newFRHF[matrixIdx] = 0.0f;
                continue;
            }

            const Position& speakerPos = localSpeakerPositions[static_cast<size_t>(outIdx)];
            const Position& listenerPos = localListenerPositions[static_cast<size_t>(outIdx)];

            // Calculate direct distance (for ratio calculation)
            float directDistance = distance3D(inputPos, speakerPos);

            // Calculate reflected distances
            float reflectedToListener = distance3D(reflectedPos, listenerPos);
            float reflectedToSpeaker = distance3D(reflectedPos, speakerPos);
            float speakerToListener = distance3D(speakerPos, listenerPos);

            // FR delay = reflected path delay - direct path delay
            // Reflected delay uses same formula as direct: (source_to_listener - speaker_to_listener) / speed
            float reflectedDelayMeters = reflectedToListener - speakerToListener;
            float reflectedDelayMs = (reflectedDelayMeters / speedOfSound) * 1000.0f;
            reflectedDelayMs = juce::jmax(0.0f, reflectedDelayMs);

            // Get direct signal delay for this routing
            float directDelayMs = newDelays[matrixIdx];

            // FR extra delay = reflected delay - direct delay
            float frExtraDelayMs = reflectedDelayMs - directDelayMs;
            frExtraDelayMs = juce::jmax(0.0f, frExtraDelayMs);

            newFRDelays[matrixIdx] = frExtraDelayMs;

            // FR level attenuation = input FR attenuation + distance ratio attenuation
            // Distance ratio attenuation = -20 * log10(reflected_distance / direct_distance)
            float distanceRatio = reflectedToSpeaker / juce::jmax(0.001f, directDistance);
            float distanceAttenDb = -20.0f * std::log10(juce::jmax(0.001f, distanceRatio));
            float totalFRAttenDb = frAttenDb + distanceAttenDb;

            // Clamp to reasonable range
            totalFRAttenDb = juce::jlimit(-92.0f, 0.0f, totalFRAttenDb);

            // Convert to linear
            float frLevelLinear = std::pow(10.0f, totalFRAttenDb / 20.0f);

            // Apply angular attenuation (recalculate for FR - same as direct signal)
            float angularAtten = calculateAngularAttenuation(inIdx, outIdx, inputPos, speakerPos);
            frLevelLinear *= angularAtten;

            newFRLevels[matrixIdx] = frLevelLinear;

            // FR HF attenuation: start with direct signal HF, add extra for longer path
            auto outputPositionSection = valueTreeState.getOutputPositionSection(outIdx);
            float outputHFdamp = outputPositionSection.getProperty(outputHFdamping, outputHFdampingDefault);

            // Extra path length for reflected signal
            float extraPathMeters = reflectedToSpeaker - directDistance;

            // FR HF = direct HF + (extra path * HF damping)
            float frHF = newHF[matrixIdx] + (outputHFdamp * extraPathMeters);
            frHF = juce::jlimit(-60.0f, 0.0f, frHF);

            newFRHF[matrixIdx] = frHF;
        }
    }

    // ==========================================================================
    // INPUT → REVERB FEED CALCULATIONS
    // ==========================================================================
    // Reverb feeds act like simplified outputs (spatial microphones)
    // They receive the common attenuation adjustment from outputs but are not
    // included in the minimum search

    // Temporary arrays for reverb feed level post-processing (per reverb, reset per input)
    std::vector<float> tempReverbAttenuationDb (static_cast<size_t> (numReverbs));
    std::vector<float> tempReverbAngularAtten (static_cast<size_t> (numReverbs));
    std::vector<bool> validReverbForMinLatency (static_cast<size_t> (numReverbs), false);

    for (int inIdx = 0; inIdx < numInputs; ++inIdx)
    {
        // Skip inputs that don't need recalculation (existing values preserved from copy)
        if (!inputsToRecalc[static_cast<size_t>(inIdx)])
            continue;

        const Position& inputPos = localInputPositions[static_cast<size_t> (inIdx)];

        // Check if this input has reverb sends muted
        if (isInputReverbMuted (inIdx))
        {
            // Zero out all reverb sends for this input
            for (int revIdx = 0; revIdx < numReverbs; ++revIdx)
            {
                const size_t matrixIdx = static_cast<size_t> (inIdx * numReverbs + revIdx);
                newInputReverbDelays[matrixIdx] = 0.0f;
                newInputReverbLevels[matrixIdx] = 0.0f;
                newInputReverbHF[matrixIdx] = 0.0f;
            }
            continue;
        }

        // Get input parameters (same as for outputs)
        auto inputAttenSection = valueTreeState.getInputAttenuationSection (inIdx);
        float inputAtten = inputAttenSection.getProperty (inputAttenuation, inputAttenuationDefault);
        float inputDistAtten = inputAttenSection.getProperty (inputDistanceAttenuation, inputDistanceAttenuationDefault);
        int attenLaw = inputAttenSection.getProperty (inputAttenuationLaw, inputAttenuationLawDefault);
        float distRatio = inputAttenSection.getProperty (inputDistanceRatio, inputDistanceRatioDefault);

        auto inputChannelSection = valueTreeState.getInputChannelSection (inIdx);
        int minimalLatencyMode = inputChannelSection.getProperty (inputMinimalLatency, 0);
        float inputDelayLat = inputChannelSection.getProperty (inputDelayLatency, 0.0f);

        auto inputPosSection = valueTreeState.getInputPositionSection (inIdx);
        int heightFactorPercent = inputPosSection.getProperty (inputHeightFactor, inputHeightFactorDefault);
        float heightFactor = static_cast<float> (heightFactorPercent) / 100.0f;

        // Get input directivity parameters
        auto inputDirectivitySection = valueTreeState.getInputDirectivitySection (inIdx);
        int directivityDeg = inputDirectivitySection.getProperty (inputDirectivity, inputDirectivityDefault);
        int rotationDeg = inputDirectivitySection.getProperty (inputRotation, inputRotationDefault);
        int tiltDeg = inputDirectivitySection.getProperty (inputTilt, inputTiltDefault);
        float hfShelfDb = inputDirectivitySection.getProperty (inputHFshelf, inputHFshelfDefault);

        float directivityRad = static_cast<float> (directivityDeg) * (juce::MathConstants<float>::pi / 180.0f);
        float rotationRad = static_cast<float> (rotationDeg) * (juce::MathConstants<float>::pi / 180.0f);
        float tiltRad = static_cast<float> (tiltDeg) * (juce::MathConstants<float>::pi / 180.0f);

        // Add gyrophone rotation offset (Leslie speaker effect for HF directivity)
        if (static_cast<size_t>(inIdx) < localGyrophoneOffsets.size())
            rotationRad += localGyrophoneOffsets[static_cast<size_t>(inIdx)];

        float facingX = std::sin (rotationRad) * std::cos (tiltRad);
        float facingY = -std::cos (rotationRad) * std::cos (tiltRad);
        float facingZ = std::sin (tiltRad);

        // Get common attenuation adjustment calculated from outputs
        float commonAttenAdjustment = inputCommonAttenAdjustments[static_cast<size_t> (inIdx)];

        // Reset tracking arrays
        std::fill (validReverbForMinLatency.begin(), validReverbForMinLatency.end(), false);
        std::fill (tempReverbAttenuationDb.begin(), tempReverbAttenuationDb.end(), -92.0f);
        std::fill (tempReverbAngularAtten.begin(), tempReverbAngularAtten.end(), 0.0f);

        for (int revIdx = 0; revIdx < numReverbs; ++revIdx)
        {
            const size_t matrixIdx = static_cast<size_t> (inIdx * numReverbs + revIdx);
            const Position& reverbFeedPos = localReverbFeedPositions[static_cast<size_t> (revIdx)];

            // Get reverb feed parameters
            auto feedSection = valueTreeState.getReverbFeedSection (revIdx);
            int reverbMiniLatEnable = feedSection.getProperty (reverbMiniLatencyEnable, reverbMiniLatencyEnableDefault);
            // Note: reverbLSenable (Live Source attenuation enable) reserved for future use
            int reverbDistAttenPercent = feedSection.getProperty (reverbDistanceAttenEnable, reverbDistanceAttenEnableDefault);
            float reverbHFdamp = feedSection.getProperty (reverbHFdamping, reverbHFdampingDefault);

            // Angular attenuation check
            float angularAtten = calculateReverbFeedAngularAttenuation (inIdx, revIdx, inputPos, reverbFeedPos);

            if (angularAtten <= 0.0f)
            {
                newInputReverbDelays[matrixIdx] = 0.0f;
                newInputReverbLevels[matrixIdx] = 0.0f;
                newInputReverbHF[matrixIdx] = 0.0f;
                continue;
            }

            // This reverb is valid for minimal latency if reverbMiniLatencyEnable = 1
            if (reverbMiniLatEnable == 1)
                validReverbForMinLatency[static_cast<size_t> (revIdx)] = true;

            // Distance calculation with height factor
            auto distanceWithHeightFactor = [heightFactor] (const Position& a, const Position& b) -> float
            {
                float dx = b.x - a.x;
                float dy = b.y - a.y;
                float dz = (b.z - a.z) * heightFactor;
                return std::sqrt (dx * dx + dy * dy + dz * dz);
            };

            float inputToReverbFeed = distanceWithHeightFactor (inputPos, reverbFeedPos);

            // Delay calculation: simple distance / speed of sound (no parallax for reverb feeds)
            float delayMs = (inputToReverbFeed / speedOfSound) * 1000.0f;
            newInputReverbDelays[matrixIdx] = juce::jmax (0.0f, delayMs);

            // Level attenuation calculation (same law as outputs)
            float distanceAttenDb = 0.0f;

            if (attenLaw == 0)
            {
                // Linear law (dB/m)
                distanceAttenDb = inputDistAtten * inputToReverbFeed;
            }
            else
            {
                // Inverse square law (1/d)
                float effectiveDistance = inputToReverbFeed / juce::jmax (0.001f, distRatio);
                if (effectiveDistance < 1.0f)
                    distanceAttenDb = 0.0f;
                else
                    distanceAttenDb = -20.0f * std::log10 (effectiveDistance);
            }

            // Apply reverb's distance attenuation percentage
            float attenuationDb = inputAtten + distanceAttenDb * (static_cast<float> (reverbDistAttenPercent) / 100.0f);
            attenuationDb = juce::jlimit (-92.0f, 0.0f, attenuationDb);

            // Store for post-processing
            tempReverbAttenuationDb[static_cast<size_t> (revIdx)] = attenuationDb;
            tempReverbAngularAtten[static_cast<size_t> (revIdx)] = angularAtten;

            // Mark as active
            newInputReverbLevels[matrixIdx] = 1.0f;

            // HF attenuation calculation
            float hfAttenReverb = reverbHFdamp * inputToReverbFeed;

            // Input directivity HF attenuation
            float hfAttenDirectivity = 0.0f;
            if (directivityDeg < 360 && hfShelfDb < 0.0f)
            {
                float dx = reverbFeedPos.x - inputPos.x;
                float dy = reverbFeedPos.y - inputPos.y;
                float dz = reverbFeedPos.z - inputPos.z;
                float rawDistance = std::sqrt (dx * dx + dy * dy + dz * dz);

                if (rawDistance > 0.001f)
                {
                    float invDist = 1.0f / rawDistance;
                    float toReverbX = dx * invDist;
                    float toReverbY = dy * invDist;
                    float toReverbZ = dz * invDist;

                    float dotProduct = facingX * toReverbX + facingY * toReverbY + facingZ * toReverbZ;
                    dotProduct = juce::jlimit (-1.0f, 1.0f, dotProduct);

                    float angleToReverb = std::acos (dotProduct);
                    float halfDirectivity = directivityRad * 0.5f;

                    if (angleToReverb > halfDirectivity)
                    {
                        float transitionRange = juce::MathConstants<float>::pi - halfDirectivity;
                        if (transitionRange > 0.001f)
                        {
                            float progress = (angleToReverb - halfDirectivity) / transitionRange;
                            progress = juce::jmin (1.0f, progress);
                            float sinArg = progress * juce::MathConstants<float>::halfPi;
                            hfAttenDirectivity = hfShelfDb * std::sqrt (std::sin (sinArg));
                        }
                    }
                }
            }

            float hfAtten = hfAttenReverb + hfAttenDirectivity;
            newInputReverbHF[matrixIdx] = juce::jlimit (-60.0f, 0.0f, hfAtten);
        }

        // Delay post-processing for reverb feeds
        if (minimalLatencyMode == 1)
        {
            // Minimal Latency Mode: find minimum delay among valid reverbs
            float minDelay = std::numeric_limits<float>::max();
            bool foundValid = false;

            for (int revIdx = 0; revIdx < numReverbs; ++revIdx)
            {
                if (validReverbForMinLatency[static_cast<size_t> (revIdx)])
                {
                    const size_t matrixIdx = static_cast<size_t> (inIdx * numReverbs + revIdx);
                    if (newInputReverbDelays[matrixIdx] < minDelay)
                    {
                        minDelay = newInputReverbDelays[matrixIdx];
                        foundValid = true;
                    }
                }
            }

            if (foundValid)
            {
                for (int revIdx = 0; revIdx < numReverbs; ++revIdx)
                {
                    const size_t matrixIdx = static_cast<size_t> (inIdx * numReverbs + revIdx);
                    if (newInputReverbLevels[matrixIdx] > 0.0f)
                    {
                        float finalDelay = newInputReverbDelays[matrixIdx] - minDelay;
                        newInputReverbDelays[matrixIdx] = juce::jmax (0.0f, finalDelay);
                    }
                }
            }
        }
        else
        {
            // Acoustic Precedence Mode: add haas/latency offsets
            for (int revIdx = 0; revIdx < numReverbs; ++revIdx)
            {
                const size_t matrixIdx = static_cast<size_t> (inIdx * numReverbs + revIdx);
                if (newInputReverbLevels[matrixIdx] > 0.0f)
                {
                    auto channelSection = valueTreeState.getReverbChannelSection (revIdx);
                    float reverbDelayLat = channelSection.getProperty (reverbDelayLatency, reverbDelayLatencyDefault);

                    float finalDelay = newInputReverbDelays[matrixIdx] + globalHaasEffect - globalSystemLatency
                                       + inputDelayLat + reverbDelayLat;
                    newInputReverbDelays[matrixIdx] = juce::jmax (0.0f, finalDelay);
                }
            }
        }

        // Apply common attenuation adjustment (from outputs) and convert to linear levels
        for (int revIdx = 0; revIdx < numReverbs; ++revIdx)
        {
            const size_t matrixIdx = static_cast<size_t> (inIdx * numReverbs + revIdx);

            if (newInputReverbLevels[matrixIdx] <= 0.0f)
                continue;

            float attenuationDb = tempReverbAttenuationDb[static_cast<size_t> (revIdx)];
            float angularAtten = tempReverbAngularAtten[static_cast<size_t> (revIdx)];

            // Apply common attenuation adjustment (clamped to 0dB - no amplification)
            attenuationDb += commonAttenAdjustment;
            attenuationDb = juce::jlimit (-92.0f, 0.0f, attenuationDb);

            float linearLevel = std::pow (10.0f, attenuationDb / 20.0f);
            linearLevel *= angularAtten;

            newInputReverbLevels[matrixIdx] = linearLevel;
        }
    }

    // ==========================================================================
    // REVERB RETURN → OUTPUT CALCULATIONS
    // ==========================================================================
    // Reverb returns act like simplified inputs (ambient sources)
    // Only recalculate if outputs or reverbs have changed (existing values preserved from copy)

    if (needOutputRecalc || needReverbRecalc)
    {
    // Temporary arrays for reverb return level post-processing
    std::vector<float> tempReverbReturnAttenDb (static_cast<size_t> (numOutputs));
    std::vector<float> tempReverbReturnAngularAtten (static_cast<size_t> (numOutputs));
    std::vector<bool> validOutputForReverbMinLatency (static_cast<size_t> (numOutputs), false);
    std::vector<bool> validOutputForReverbCommonAtten (static_cast<size_t> (numOutputs), false);

    for (int revIdx = 0; revIdx < numReverbs; ++revIdx)
    {
        const Position& returnPos = localReverbReturnPositions[static_cast<size_t> (revIdx)];

        // Get reverb return parameters
        auto returnSection = valueTreeState.getReverbReturnSection (revIdx);
        float reverbDistAtten = returnSection.getProperty (reverbDistanceAttenuation, reverbDistanceAttenuationDefault);
        int reverbCommonAttenPct = returnSection.getProperty (reverbCommonAtten, reverbCommonAttenDefault);
        float reverbCommonAttenFactor = static_cast<float> (reverbCommonAttenPct) / 100.0f;

        auto feedSection = valueTreeState.getReverbFeedSection (revIdx);
        int reverbMiniLatEnable = feedSection.getProperty (reverbMiniLatencyEnable, reverbMiniLatencyEnableDefault);

        // Reset tracking arrays
        std::fill (validOutputForReverbMinLatency.begin(), validOutputForReverbMinLatency.end(), false);
        std::fill (validOutputForReverbCommonAtten.begin(), validOutputForReverbCommonAtten.end(), false);
        std::fill (tempReverbReturnAttenDb.begin(), tempReverbReturnAttenDb.end(), -92.0f);
        std::fill (tempReverbReturnAngularAtten.begin(), tempReverbReturnAngularAtten.end(), 0.0f);

        for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
        {
            const size_t matrixIdx = static_cast<size_t> (revIdx * numOutputs + outIdx);

            // Check reverb→output mute
            if (isReverbOutputMuted (revIdx, outIdx))
            {
                newReverbOutputDelays[matrixIdx] = 0.0f;
                newReverbOutputLevels[matrixIdx] = 0.0f;
                newReverbOutputHF[matrixIdx] = 0.0f;
                continue;
            }

            const Position& speakerPos = localSpeakerPositions[static_cast<size_t> (outIdx)];
            const Position& listenerPos = localListenerPositions[static_cast<size_t> (outIdx)];

            // Get output parameters
            auto outputOptionsSection = valueTreeState.getOutputOptionsSection (outIdx);
            auto outputPositionSection = valueTreeState.getOutputPositionSection (outIdx);

            int outputMiniLatEnable = outputOptionsSection.getProperty (outputMiniLatencyEnable, 1);
            float outputDistAttenPercent = outputOptionsSection.getProperty (outputDistanceAttenPercent, 100.0f);
            float outputHFdamp = outputPositionSection.getProperty (outputHFdamping, outputHFdampingDefault);

            // Angular attenuation: use output's angular parameters on reverb return position
            // (reverb returns are treated like inputs - can be screened by speaker facing)
            float angularAtten = calculateAngularAttenuation (-1, outIdx, returnPos, speakerPos);

            if (angularAtten <= 0.0f)
            {
                newReverbOutputDelays[matrixIdx] = 0.0f;
                newReverbOutputLevels[matrixIdx] = 0.0f;
                newReverbOutputHF[matrixIdx] = 0.0f;
                continue;
            }

            // Track validity for minimal latency and common attenuation
            if (outputMiniLatEnable == 1 && reverbMiniLatEnable == 1)
                validOutputForReverbMinLatency[static_cast<size_t> (outIdx)] = true;

            validOutputForReverbCommonAtten[static_cast<size_t> (outIdx)] = true;

            // Distance calculations
            float returnToSpeaker = distance3D (returnPos, speakerPos);
            float returnToListener = distance3D (returnPos, listenerPos);
            float speakerToListener = distance3D (speakerPos, listenerPos);

            // Delay calculation with parallax
            float delayMeters = returnToListener - speakerToListener;
            float delayMs = (delayMeters / speedOfSound) * 1000.0f;
            newReverbOutputDelays[matrixIdx] = juce::jmax (0.0f, delayMs);

            // Level attenuation: simple dB/m rate (no law switching)
            float attenuationDb = reverbDistAtten * returnToSpeaker;
            attenuationDb *= (outputDistAttenPercent / 100.0f);
            attenuationDb = juce::jlimit (-92.0f, 0.0f, attenuationDb);

            tempReverbReturnAttenDb[static_cast<size_t> (outIdx)] = attenuationDb;
            tempReverbReturnAngularAtten[static_cast<size_t> (outIdx)] = angularAtten;
            newReverbOutputLevels[matrixIdx] = 1.0f;  // Mark as active

            // HF attenuation (output HF damping only, no directivity for reverb returns)
            float hfAtten = outputHFdamp * returnToSpeaker;
            newReverbOutputHF[matrixIdx] = juce::jlimit (-60.0f, 0.0f, hfAtten);
        }

        // Delay post-processing for reverb returns
        // Find minimum delay among valid outputs
        float minDelay = std::numeric_limits<float>::max();
        bool foundValid = false;

        for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
        {
            if (validOutputForReverbMinLatency[static_cast<size_t> (outIdx)])
            {
                const size_t matrixIdx = static_cast<size_t> (revIdx * numOutputs + outIdx);
                if (newReverbOutputDelays[matrixIdx] < minDelay)
                {
                    minDelay = newReverbOutputDelays[matrixIdx];
                    foundValid = true;
                }
            }
        }

        if (foundValid)
        {
            for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
            {
                const size_t matrixIdx = static_cast<size_t> (revIdx * numOutputs + outIdx);
                if (newReverbOutputLevels[matrixIdx] > 0.0f)
                {
                    float finalDelay = newReverbOutputDelays[matrixIdx] - minDelay;
                    newReverbOutputDelays[matrixIdx] = juce::jmax (0.0f, finalDelay);
                }
            }
        }

        // Level post-processing: apply reverbCommonAtten
        // Note: 100% = keep full original attenuation (no lift), 0% = apply full lift
        float reverbCommonAttenAdjust = 0.0f;

        if (reverbCommonAttenFactor < 1.0f)
        {
            // Find minimum attenuation among valid outputs
            float minAttenuation = -92.0f;
            bool foundValidForCommon = false;

            for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
            {
                if (validOutputForReverbCommonAtten[static_cast<size_t> (outIdx)])
                {
                    float atten = tempReverbReturnAttenDb[static_cast<size_t> (outIdx)];
                    if (atten > minAttenuation)
                    {
                        minAttenuation = atten;
                        foundValidForCommon = true;
                    }
                }
            }

            // Invert factor: 0% → full lift, 100% → no lift
            if (foundValidForCommon)
                reverbCommonAttenAdjust = -minAttenuation * (1.0f - reverbCommonAttenFactor);
        }

        // Apply common attenuation and convert to linear
        for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
        {
            const size_t matrixIdx = static_cast<size_t> (revIdx * numOutputs + outIdx);

            if (newReverbOutputLevels[matrixIdx] <= 0.0f)
                continue;

            float attenuationDb = tempReverbReturnAttenDb[static_cast<size_t> (outIdx)];
            float angularAtten = tempReverbReturnAngularAtten[static_cast<size_t> (outIdx)];

            attenuationDb += reverbCommonAttenAdjust;
            attenuationDb = juce::jlimit (-92.0f, 0.0f, attenuationDb);

            float linearLevel = std::pow (10.0f, attenuationDb / 20.0f);
            linearLevel *= angularAtten;

            newReverbOutputLevels[matrixIdx] = linearLevel;
        }
    }
    } // End of if (needOutputRecalc || needReverbRecalc)

    // Update all matrices under lock
    {
        const juce::ScopedLock sl (matrixLock);

        // Input → Output
        delayTimesMs = std::move (newDelays);
        levels = std::move (newLevels);
        hfAttenuationDb = std::move (newHF);

        // Floor Reflections
        frDelayTimesMs = std::move (newFRDelays);
        frLevels = std::move (newFRLevels);
        frHFAttenuationDb = std::move (newFRHF);

        // Input → Reverb Feed
        inputReverbDelayTimesMs = std::move (newInputReverbDelays);
        inputReverbLevels = std::move (newInputReverbLevels);
        inputReverbHFAttenuationDb = std::move (newInputReverbHF);

        // Reverb Return → Output
        reverbOutputDelayTimesMs = std::move (newReverbOutputDelays);
        reverbOutputLevels = std::move (newReverbOutputLevels);
        reverbOutputHFAttenuationDb = std::move (newReverbOutputHF);
    }

    // Update ramp states under position lock
    {
        const juce::ScopedLock sl (positionLock);
        // Delay mode ramp state
        previousMinimalLatencyMode = std::move (localPreviousMode);
        delayModeRampOffset = std::move (localRampOffset);
        // Common attenuation ramp state
        previousCommonAttenPercent = std::move (localPrevCommonAttenPercent);
        commonAttenRampOffsetDb = std::move (localCommonAttenRampOffsetDb);
        commonAttenRampTimeRemaining = std::move (localCommonAttenRampTimeRemaining);
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

int WFSCalculationEngine::findReverbIndexFromTree (const juce::ValueTree& tree) const
{
    juce::ValueTree current = tree;

    while (current.isValid())
    {
        if (current.getType() == Reverb)
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
            // Output changed - affects ALL inputs, need full recalc
            outputsDirty.store(true);
            matrixDirty.store(true);
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
            // Mark only this specific input as dirty
            inputDirtyFlags[static_cast<size_t> (inputIndex)] = true;
            matrixDirty.store(true);
        }
        return;
    }

    // Reverb position properties
    bool isReverbPositionProperty = (property == reverbPositionX ||
                                     property == reverbPositionY ||
                                     property == reverbPositionZ);

    bool isReverbReturnOffsetProperty = (property == reverbReturnOffsetX ||
                                         property == reverbReturnOffsetY ||
                                         property == reverbReturnOffsetZ);

    if (isReverbPositionProperty || isReverbReturnOffsetProperty)
    {
        int reverbIndex = findReverbIndexFromTree (tree);

        if (reverbIndex >= 0 && reverbIndex < numReverbs)
        {
            const juce::ScopedLock sl (positionLock);

            if (isReverbPositionProperty)
            {
                updateReverbFeedPosition (reverbIndex);
                updateReverbReturnPosition (reverbIndex);  // Return depends on feed
            }
            else
            {
                updateReverbReturnPosition (reverbIndex);
            }
            // Reverb changed - affects ALL inputs for input→reverb matrix
            reverbsDirty.store(true);
            matrixDirty.store(true);
        }
        return;
    }

    // ==========================================================================
    // INPUT PARAMETERS THAT AFFECT CALCULATIONS
    // ==========================================================================

    // Input attenuation parameters
    bool isInputAttenProperty = (property == inputAttenuation ||
                                 property == inputDistanceAttenuation ||
                                 property == inputAttenuationLaw ||
                                 property == inputDistanceRatio ||
                                 property == inputCommonAtten);

    // Input channel parameters
    bool isInputChannelProperty = (property == inputMinimalLatency ||
                                   property == inputDelayLatency);

    // Input height factor
    bool isInputHeightProperty = (property == inputHeightFactor);

    // Input directivity parameters
    bool isInputDirectivityProperty = (property == inputDirectivity ||
                                       property == inputRotation ||
                                       property == inputTilt ||
                                       property == inputHFshelf);

    // Input mute, sidelines, and array attenuation parameters
    bool isInputMuteProperty = (property == inputMutes ||
                                property == inputMuteReverbSends ||
                                property == inputSidelinesActive ||
                                property == inputSidelinesFringe ||
                                property == inputArrayAtten1 ||
                                property == inputArrayAtten2 ||
                                property == inputArrayAtten3 ||
                                property == inputArrayAtten4 ||
                                property == inputArrayAtten5 ||
                                property == inputArrayAtten6 ||
                                property == inputArrayAtten7 ||
                                property == inputArrayAtten8 ||
                                property == inputArrayAtten9 ||
                                property == inputArrayAtten10);

    // Live Source Tamer parameters (affect level calculations via sharedLSGains)
    bool isInputLSProperty = (property == inputLSactive ||
                              property == inputLSradius ||
                              property == inputLSshape ||
                              property == inputLSattenuation);

    // Flip parameters (mirror position around origin)
    bool isInputFlipProperty = (property == inputFlipX ||
                                property == inputFlipY ||
                                property == inputFlipZ);

    // Offset parameters (added to position after flip)
    bool isInputOffsetProperty = (property == inputOffsetX ||
                                  property == inputOffsetY ||
                                  property == inputOffsetZ);

    if (isInputAttenProperty || isInputChannelProperty || isInputHeightProperty ||
        isInputDirectivityProperty || isInputMuteProperty || isInputLSProperty ||
        isInputFlipProperty || isInputOffsetProperty)
    {
        int inputIndex = findInputIndexFromTree (tree);

        if (inputIndex >= 0 && inputIndex < numInputs)
        {
            const juce::ScopedLock sl (positionLock);
            inputDirtyFlags[static_cast<size_t> (inputIndex)] = true;
            matrixDirty.store(true);
        }
        return;
    }

    // ==========================================================================
    // OUTPUT PARAMETERS THAT AFFECT CALCULATIONS
    // ==========================================================================

    // Output options that affect calculations
    bool isOutputOptionProperty = (property == outputMiniLatencyEnable ||
                                   property == outputDistanceAttenPercent ||
                                   property == outputDelayLatency ||
                                   property == outputArray ||      // Array assignment affects per-array attenuation
                                   property == outputLSattenEnable);  // Live Source Tamer per-output enable

    // Output angular parameters
    bool isOutputAngularProperty = (property == outputPitch ||
                                    property == outputAngleOn ||
                                    property == outputAngleOff ||
                                    property == outputHFdamping);

    if (isOutputOptionProperty || isOutputAngularProperty)
    {
        // Output parameter changed - affects ALL inputs
        outputsDirty.store(true);
        matrixDirty.store(true);
        return;
    }

    // ==========================================================================
    // REVERB PARAMETERS THAT AFFECT CALCULATIONS
    // ==========================================================================

    // Reverb feed angular parameters
    bool isReverbFeedProperty = (property == reverbOrientation ||
                                 property == reverbPitch ||
                                 property == reverbAngleOn ||
                                 property == reverbAngleOff ||
                                 property == reverbMiniLatencyEnable ||
                                 property == reverbDistanceAttenEnable ||
                                 property == reverbHFdamping);

    // Reverb return parameters
    bool isReverbReturnProperty = (property == reverbDistanceAttenuation ||
                                   property == reverbCommonAtten ||
                                   property == reverbDelayLatency ||
                                   property == reverbMutes);

    if (isReverbFeedProperty || isReverbReturnProperty)
    {
        // Reverb parameter changed - affects ALL inputs for reverb matrices
        reverbsDirty.store(true);
        matrixDirty.store(true);
        return;
    }

    // ==========================================================================
    // GLOBAL/MASTER PARAMETERS THAT AFFECT ALL CALCULATIONS
    // ==========================================================================

    bool isMasterProperty = (property == haasEffect ||
                             property == systemLatency);

    if (isMasterProperty)
    {
        // Global parameter changed - affects everything
        outputsDirty.store(true);
        reverbsDirty.store(true);
        matrixDirty.store(true);
        return;
    }
}
