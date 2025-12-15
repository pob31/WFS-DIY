#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"

//==============================================================================
/**
    WFS Calculation Engine

    Calculates all DSP parameters for the WFS matrix:
    - Listener positions (where each speaker aims)
    - Input positions (source locations)
    - Delay times (based on path length differences)
    - Levels (based on distance attenuation)
    - HF attenuation (air absorption)

    Coordinate System:
    - X: Across stage (left-right)
    - Y: Positive upstage (away from audience), 0° orientation faces -Y
    - Z: Height

    Update Strategy:
    - Listener/speaker positions: Cached, update on output param change
    - Input positions: Cached, update on input param change
    - Matrix (delays/levels/HF): Recalculated on demand via recalculateMatrix()
*/
class WFSCalculationEngine : private juce::ValueTree::Listener
{
public:
    //==========================================================================
    struct Position
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    //==========================================================================
    explicit WFSCalculationEngine (WFSValueTreeState& state);
    ~WFSCalculationEngine() override;

    //==========================================================================
    // Position Access (thread-safe)
    //==========================================================================

    /** Get cached listener position for an output */
    Position getListenerPosition (int outputIndex) const;

    /** Get cached speaker position for an output */
    Position getSpeakerPosition (int outputIndex) const;

    /** Get cached input position */
    Position getInputPosition (int inputIndex) const;

    /** Force recalculation of all listener positions */
    void recalculateAllListenerPositions();

    /** Force recalculation of all input positions */
    void recalculateAllInputPositions();

    //==========================================================================
    // Matrix Calculation (call at 50Hz from timer)
    //==========================================================================

    /** Recalculate entire delay/level/HF matrix. Call this at control rate (~50Hz) */
    void recalculateMatrix();

    /** Get matrix dimensions */
    int getNumInputs() const { return numInputs; }
    int getNumOutputs() const { return numOutputs; }

    //==========================================================================
    // Matrix Results (thread-safe read)
    // Index: [inputIndex * numOutputs + outputIndex]
    //==========================================================================

    /** Get pointer to delay times array (ms). Size = numInputs * numOutputs */
    const float* getDelayTimesMs() const { return delayTimesMs.data(); }

    /** Get pointer to levels array (linear 0-1). Size = numInputs * numOutputs */
    const float* getLevels() const { return levels.data(); }

    /** Get pointer to HF attenuation array (dB, negative). Size = numInputs * numOutputs */
    const float* getHFAttenuationDb() const { return hfAttenuationDb.data(); }

    /** Get delay for specific routing */
    float getDelayMs (int inputIndex, int outputIndex) const;

    /** Get level for specific routing */
    float getLevel (int inputIndex, int outputIndex) const;

    /** Get HF attenuation for specific routing */
    float getHFAttenuation (int inputIndex, int outputIndex) const;

private:
    //==========================================================================
    // ValueTree::Listener overrides
    //==========================================================================
    void valueTreePropertyChanged (juce::ValueTree& tree,
                                   const juce::Identifier& property) override;
    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override {}
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override {}
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged (juce::ValueTree&) override {}

    //==========================================================================
    // Internal calculation methods
    //==========================================================================
    void recalculateListenerPosition (int outputIndex);
    void updateSpeakerPosition (int outputIndex);
    void updateInputPosition (int inputIndex);
    int findOutputIndexFromTree (const juce::ValueTree& tree) const;
    int findInputIndexFromTree (const juce::ValueTree& tree) const;

    static float distance3D (const Position& a, const Position& b);

    /** Check if input→output routing is muted */
    bool isRoutingMuted (int inputIndex, int outputIndex) const;

    /** Calculate angular attenuation for input→output pair.
        Returns 0.0 (muted) to 1.0 (full).
        Based on speaker orientation, pitch, angleOn, angleOff. */
    float calculateAngularAttenuation (int inputIndex, int outputIndex,
                                       const Position& inputPos,
                                       const Position& speakerPos) const;

    //==========================================================================
    // State
    //==========================================================================
    WFSValueTreeState& valueTreeState;
    int numInputs = 0;
    int numOutputs = 0;

    // Cached positions
    std::vector<Position> listenerPositions;   // [outputIndex]
    std::vector<Position> speakerPositions;    // [outputIndex]
    std::vector<Position> inputPositions;      // [inputIndex]

    // Matrix results [inputIndex * numOutputs + outputIndex]
    std::vector<float> delayTimesMs;
    std::vector<float> levels;
    std::vector<float> hfAttenuationDb;

    // Thread safety
    mutable juce::CriticalSection positionLock;
    mutable juce::CriticalSection matrixLock;

    // Speed of sound (m/s)
    static constexpr float speedOfSound = 343.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WFSCalculationEngine)
};
