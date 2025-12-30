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
    // LFO Offset Support
    //==========================================================================

    /** Set LFO offset for an input (called from LFOProcessor at 50Hz) */
    void setLFOOffset (int inputIndex, float x, float y, float z);

    /** Get LFO offset for an input (for UI visualization) */
    Position getLFOOffset (int inputIndex) const;

    /** Set gyrophone rotation offset for an input (radians, added to rotation for HF directivity) */
    void setGyrophoneOffset (int inputIndex, float offsetRad);

    /** Get gyrophone rotation offset for an input */
    float getGyrophoneOffset (int inputIndex) const;

    //==========================================================================
    // Speed-Limited Position Support
    //==========================================================================

    /** Set speed-limited position for an input (called from InputSpeedLimiter at 50Hz).
        This is the interpolated position that moves smoothly towards the target. */
    void setSpeedLimitedPosition (int inputIndex, float x, float y, float z);

    /** Get speed-limited position for an input (for UI visualization) */
    Position getSpeedLimitedPosition (int inputIndex) const;

    //==========================================================================
    // Delay Mode Ramp Support
    //==========================================================================

    /** Update delay mode ramps - call at 50Hz to decay ramp offsets over 1 second */
    void updateDelayModeRamps (float deltaTimeSeconds);

    //==========================================================================
    // Matrix Calculation (call at 50Hz from timer)
    //==========================================================================

    /** Recalculate entire delay/level/HF matrix if dirty. Call this at control rate (~50Hz)
        Returns true if recalculation was performed, false if skipped (not dirty) */
    bool recalculateMatrixIfDirty();

    /** Force recalculation regardless of dirty state */
    void recalculateMatrix();

    /** Check if matrix needs recalculation */
    bool isMatrixDirty() const { return matrixDirty.load(); }

    /** Mark matrix as needing recalculation */
    void markMatrixDirty() { matrixDirty.store(true); }

    /** Mark all inputs as needing recalculation (e.g., when LS gains change) */
    void markAllInputsDirty();

    /** Get matrix dimensions */
    int getNumInputs() const { return numInputs; }
    int getNumOutputs() const { return numOutputs; }
    int getNumReverbs() const { return numReverbs; }

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

    //==========================================================================
    // Floor Reflection Matrix Results (thread-safe read)
    // Index: [inputIndex * numOutputs + outputIndex]
    //==========================================================================

    /** Get pointer to FR delay times array (ms, extra delay for reflected path).
        Size = numInputs * numOutputs */
    const float* getFRDelayTimesMs() const { return frDelayTimesMs.data(); }

    /** Get pointer to FR levels array (linear 0-1).
        Size = numInputs * numOutputs */
    const float* getFRLevels() const { return frLevels.data(); }

    /** Get pointer to FR HF attenuation array (dB, negative).
        Size = numInputs * numOutputs */
    const float* getFRHFAttenuationDb() const { return frHFAttenuationDb.data(); }

    //==========================================================================
    // Live Source Tamer Integration
    //==========================================================================

    /** Set pointer to LS gains array (owned by LiveSourceTamerEngine).
        Index: [inputIndex * numOutputs + outputIndex]
        The gains are applied during level calculation in recalculateMatrix(). */
    void setLSGainsPtr(const float* ptr) { sharedLSGains = ptr; }

    //==========================================================================
    // Reverb Position Access (thread-safe)
    //==========================================================================

    /** Get cached reverb feed position */
    Position getReverbFeedPosition (int reverbIndex) const;

    /** Get cached reverb return position (feed + offset) */
    Position getReverbReturnPosition (int reverbIndex) const;

    /** Force recalculation of all reverb positions */
    void recalculateAllReverbPositions();

    //==========================================================================
    // Input → Reverb Feed Matrix Results (thread-safe read)
    // Index: [inputIndex * numReverbs + reverbIndex]
    //==========================================================================

    /** Get pointer to input→reverb delay times array (ms). Size = numInputs * numReverbs */
    const float* getInputReverbDelayTimesMs() const { return inputReverbDelayTimesMs.data(); }

    /** Get pointer to input→reverb levels array (linear 0-1). Size = numInputs * numReverbs */
    const float* getInputReverbLevels() const { return inputReverbLevels.data(); }

    /** Get pointer to input→reverb HF attenuation array (dB). Size = numInputs * numReverbs */
    const float* getInputReverbHFAttenuationDb() const { return inputReverbHFAttenuationDb.data(); }

    //==========================================================================
    // Reverb Return → Output Matrix Results (thread-safe read)
    // Index: [reverbIndex * numOutputs + outputIndex]
    //==========================================================================

    /** Get pointer to reverb→output delay times array (ms). Size = numReverbs * numOutputs */
    const float* getReverbOutputDelayTimesMs() const { return reverbOutputDelayTimesMs.data(); }

    /** Get pointer to reverb→output levels array (linear 0-1). Size = numReverbs * numOutputs */
    const float* getReverbOutputLevels() const { return reverbOutputLevels.data(); }

    /** Get pointer to reverb→output HF attenuation array (dB). Size = numReverbs * numOutputs */
    const float* getReverbOutputHFAttenuationDb() const { return reverbOutputHFAttenuationDb.data(); }

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

    /** Calculate sideline attenuation factor (0.0 = muted, 1.0 = full) for an input.
        Based on stage shape (box vs cylinder/dome) and source position relative to edges.
        Box: attenuates near left, right, and upstage edges (NOT downstage).
        Cylinder/Dome: attenuates near circular edge based on radial distance. */
    float calculateSidelineAttenuation (int inputIndex, const Position& inputPos) const;

    /** Calculate angular attenuation for input→output pair.
        Returns 0.0 (muted) to 1.0 (full).
        Based on speaker orientation, pitch, angleOn, angleOff. */
    float calculateAngularAttenuation (int inputIndex, int outputIndex,
                                       const Position& inputPos,
                                       const Position& speakerPos) const;

    /** Calculate angular attenuation for input→reverb feed pair.
        Similar to output but uses reverb feed parameters. */
    float calculateReverbFeedAngularAttenuation (int inputIndex, int reverbIndex,
                                                  const Position& inputPos,
                                                  const Position& reverbFeedPos) const;

    /** Update reverb feed position from parameters */
    void updateReverbFeedPosition (int reverbIndex);

    /** Update reverb return position from parameters (feed + offset) */
    void updateReverbReturnPosition (int reverbIndex);

    /** Find reverb index from ValueTree */
    int findReverbIndexFromTree (const juce::ValueTree& tree) const;

    /** Check if input→reverb routing is muted (inputMuteReverbSends) */
    bool isInputReverbMuted (int inputIndex) const;

    /** Check if reverb→output routing is muted (reverbMutes array) */
    bool isReverbOutputMuted (int reverbIndex, int outputIndex) const;

    //==========================================================================
    // State
    //==========================================================================
    WFSValueTreeState& valueTreeState;
    int numInputs = 0;
    int numOutputs = 0;
    int numReverbs = 0;

    // Cached positions
    std::vector<Position> listenerPositions;       // [outputIndex]
    std::vector<Position> speakerPositions;        // [outputIndex]
    std::vector<Position> inputPositions;          // [inputIndex] - Raw target positions from ValueTree
    std::vector<Position> speedLimitedPositions;   // [inputIndex] - Speed-limited interpolated positions
    std::vector<Position> reverbFeedPositions;     // [reverbIndex]
    std::vector<Position> reverbReturnPositions;   // [reverbIndex]
    std::vector<Position> lfoOffsets;              // [inputIndex] - LFO position offsets
    std::vector<float> gyrophoneOffsets;           // [inputIndex] - Gyrophone rotation offsets (radians)

    // Delay mode ramp state for smooth transitions when toggling inputMinimalLatency
    std::vector<int> previousMinimalLatencyMode;  // [inputIndex] - Previous mode (0 or 1), -1 = uninitialized
    std::vector<float> delayModeRampOffset;       // [inputIndex] - Current ramp offset in ms (decays to 0 over 1s)

    // Common attenuation ramp state for smooth transitions when changing inputCommonAtten
    std::vector<float> previousCommonAttenPercent;  // [inputIndex] - Previous common atten % (-1 = uninitialized)
    std::vector<float> commonAttenRampOffsetDb;     // [inputIndex] - Current ramp offset in dB (decays to 0)
    std::vector<float> commonAttenRampTimeRemaining; // [inputIndex] - Remaining ramp time in seconds

    // Input → Output matrix results [inputIndex * numOutputs + outputIndex]
    std::vector<float> delayTimesMs;
    std::vector<float> levels;
    std::vector<float> hfAttenuationDb;

    // Floor Reflection matrix results [inputIndex * numOutputs + outputIndex]
    // Extra delay, level, and HF for reflected signal through floor (z=0 plane)
    std::vector<float> frDelayTimesMs;      // Extra delay in ms for reflected path
    std::vector<float> frLevels;            // Linear gain (0-1) for reflected signal
    std::vector<float> frHFAttenuationDb;   // HF attenuation in dB for reflected path

    // Input → Reverb Feed matrix results [inputIndex * numReverbs + reverbIndex]
    std::vector<float> inputReverbDelayTimesMs;
    std::vector<float> inputReverbLevels;
    std::vector<float> inputReverbHFAttenuationDb;

    // Reverb Return → Output matrix results [reverbIndex * numOutputs + outputIndex]
    std::vector<float> reverbOutputDelayTimesMs;
    std::vector<float> reverbOutputLevels;
    std::vector<float> reverbOutputHFAttenuationDb;

    // Live Source Tamer gains (owned by LiveSourceTamerEngine)
    // Index: [inputIndex * numOutputs + outputIndex]
    const float* sharedLSGains = nullptr;

    // Thread safety
    mutable juce::CriticalSection positionLock;
    mutable juce::CriticalSection matrixLock;

    // Dirty flags for lazy recalculation
    std::atomic<bool> matrixDirty { true };           // Any change requiring full recalc
    std::atomic<bool> outputsDirty { true };          // Output positions changed (affects all inputs)
    std::atomic<bool> reverbsDirty { true };          // Reverb positions changed
    std::vector<bool> inputDirtyFlags;                // Per-input dirty flags (protected by positionLock)

    // Speed of sound (m/s)
    static constexpr float speedOfSound = 343.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WFSCalculationEngine)
};
