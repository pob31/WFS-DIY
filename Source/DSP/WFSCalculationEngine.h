#pragma once

#include <JuceHeader.h>
#include "../../spatcore/wfs/RenderSourceMap.h"
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include <array>
#include <atomic>
#include <cstdint>

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

    /** Get composite input position (speed-limited + flip + offset + LFO).
        This is the final position used for all calculations including Live Source Tamer. */
    Position getCompositeInputPosition (int inputIndex) const;

    //==========================================================================
    // Render sources (stereo-pair slice slots)
    //==========================================================================

    /** Install the render-source slot map. Config-time only (a channel-type
        change, which is stopped-only) — never 50 Hz: it redefines which matrix
        rows exist. Marks everything dirty. */
    void setRenderSourceMap (const spatcore::wfs::RenderSourceMap& map);

    /** 50 Hz slice geometry for one stereo-pair channel: position offsets from
        the anchor, per-slice gains and active flags for all 6 slices (slice 0
        is the channel's primary slot). Arrays are slice-major: offsetsXYZ has
        6*3 floats, gainsLinear and active have 6 entries. No-op for a mono
        channel. Marks the channel dirty only when a value actually changed. */
    void setSliceGeometry (int inputChannel,
                           const float* offsetsXYZ,
                           const float* gainsLinear,
                           const bool* active);

    /** Composite position of a render source: the owning channel's composite
        position plus the slice offset (zero for mono sources, so this is
        bit-identical to getCompositeInputPosition there). Thread-safe. */
    Position getRenderSourcePosition (int sourceIndex) const;

    /** Owning input channel of a render source (identity for mono sources and
        stereo primaries), or -1 out of range. Thread-safe. */
    int getOwningInputChannel (int sourceIndex) const;

    /** Slice gain for a render source: 1 for mono sources, the slice
        gainLinear for stereo slices, 0 for an inactive slot. Thread-safe. */
    float getRenderSourceGain (int sourceIndex) const;

    /** Intrinsic processing latency of one channel's decomposition backend
        (ms; 0 for mono channels and the Phase-0 pass-through). All render
        sources of a channel share it. The engine aligns every channel to the
        MAXIMUM across channels by ADDING (max − own) to its delays — the term
        is never negative, so it cannot fight the delay clamp at zero. Marks
        everything dirty on change (the reference is global). */
    void setChannelIntrinsicLatency (int inputChannel, float latencyMs);

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

    /** Set transient sampler cell offset for an input (added to composite position) */
    void setSamplerCellOffset (int inputIndex, float x, float y, float z);

    /** Set gyrophone rotation offset for an input (radians, added to rotation for HF directivity) */
    void setGyrophoneOffset (int inputIndex, float offsetRad);

    /** Get gyrophone rotation offset for an input */
    float getGyrophoneOffset (int inputIndex) const;

    //==========================================================================
    // Gradient Map Offset Support
    //==========================================================================

    /** Gradient map parameter offsets (runtime-only, not in ValueTree) */
    struct GradientMapOffsets
    {
        float attenuationDb = 0.0f;
        float heightMeters  = 0.0f;
        float hfShelfDb     = 0.0f;
    };

    /** Set gradient map offsets for an input (called from GUI timer at 50Hz) */
    void setGradientMapOffsets (int inputIndex, float attenDb, float heightM, float hfDb);

    /** Get gradient map offsets for an input */
    GradientMapOffsets getGradientMapOffsets (int inputIndex) const;

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
        lsGains: Live Source Tamer gains applied during level calculation, or nullptr for
        unity. Index: [inputIndex * numOutputs + outputIndex]. Supplied fresh on every call
        (never cached) so the engine holds no pointer into LiveSourceTamerEngine.
        Returns true if recalculation was performed, false if skipped (not dirty) */
    bool recalculateMatrixIfDirty (const float* lsGains);

    /** Force recalculation regardless of dirty state.
        See recalculateMatrixIfDirty() for the lsGains contract. */
    void recalculateMatrix (const float* lsGains);

    /** Check if matrix needs recalculation */
    bool isMatrixDirty() const { return matrixDirty.load(); }

    /** Mark matrix as needing recalculation */
    void markMatrixDirty() { matrixDirty.store(true); }

    /** Mark all inputs as needing recalculation (e.g., when LS gains change) */
    void markAllInputsDirty();

    /** Mark a single input as needing recalculation */
    void markInputDirty(int inputIndex);

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

    //==========================================================================
    // Effects channels
    //==========================================================================
    // An effect return is a render source: its rows of the in x out and the
    // in x reverb matrices are computed here, at slots [firstEffectSlot, count)
    // of the installed render-source map, by the same delay-and-sum contract
    // as an input's. Its FEED is the source x effect matrix below, which the
    // effects engine consumes exactly as the reverb feed thread consumes the
    // in x reverb triplet. Two positions per effect: the FEED position (the
    // channel's base position, what its feed geometry is computed against)
    // and the RETURN position (base + return offset + the AutomOtion offset),
    // which is where the return rows and the effect-to-effect feeds travel
    // from. Keeping the feed on the base position is what stops a moving
    // return from chasing its own trigger level.

    /** Cached effect feed position (the channel's base position). */
    Position getEffectFeedPosition (int effectIndex) const;

    /** Composite effect return position: base + return offset + AutomOtion
        offset, as last rendered. */
    Position getEffectReturnPosition (int effectIndex) const;

    /** Re-read every effect position from the tree; marks the effects dirty.
        Call after the effect count changed or a project was loaded. */
    void recalculateAllEffectPositions();

    /** AutomOtion offset of an effect return (50 Hz; the twin of setLFOOffset).
        Moves the RETURN only. Marks the return rows dirty when it changed. */
    void setEffectOtomoOffset (int effectIndex, float x, float y, float z);
    Position getEffectOtomoOffset (int effectIndex) const;

    /** LFO offset of an effect return (50 Hz; the twin of setEffectOtomoOffset).
        The two offsets ADD: the return renders at base + AutomOtion + LFO, and
        the feed keeps reading the base position. */
    void setEffectLFOOffset (int effectIndex, float x, float y, float z);
    Position getEffectLFOOffset (int effectIndex) const;

    /** AutomOtion + LFO: the whole movement, which is what the Map's grey dot shows. */
    Position getEffectMovementOffset (int effectIndex) const;

    /** Global "solo effects": every INPUT row of the in x out matrix is zeroed
        so only the effect returns reach the speakers. The feeds are not masked
        (solo is monitoring, not routing) and the reverb feeds keep running,
        which mirrors soloReverbs. Marks every input dirty when it changed. */
    void setSoloEffects (bool soloed);
    bool getSoloEffects() const { return soloEffectsGlobal.load(); }

    /** Kind of a render source: Input for every input slot and derived slice,
        EffectReturn for a return row; Input when out of range. Thread-safe. */
    spatcore::wfs::SourceKind getSourceKind (int sourceIndex) const;

    /** Effect channel owning a return row, or -1 for any other source. */
    int getOwningEffectChannel (int sourceIndex) const;

    /** The stride of the source x effect matrices: the effects BUDGET, never
        the live count - the engine reads cells at [source * stride + effect]
        and is told this stride, so the two cannot disagree. */
    int getNumEffects() const { return numEffects; }

    /** Source x effect feed matrices (thread-safe read while the pointer is
        held; the vectors are never reallocated). Index
        [sourceIndex * numEffects + effectIndex]; rows = every render source
        (inputs, derived slices, effect returns), size maxRenderSources *
        numEffects. The level already carries geometry x the user send cell x
        the on/off switch. The delay is carried even for a CLOSED cell, so the
        engine's tap does not teleport when the cell opens. */
    const float* getInputEffectDelayTimesMs() const { return inputEffectDelayTimesMs.data(); }
    const float* getInputEffectLevels() const { return inputEffectLevels.data(); }
    const float* getInputEffectHFAttenuationDb() const { return inputEffectHFAttenuationDb.data(); }

    /** Bit n set while effect n sits inside a cycle of the effect-to-effect
        on-switch graph (A feeds B feeds A). Recomputed whenever a send row
        changes; message-thread state for a badge and a log line, never a tree
        property. The audio side takes no action on it - the user owns the
        loops, and the engine's loop guard handles runaway. */
    uint32_t getEffectCycleMask() const { return effectCycleMask.load(); }

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

    /** Apply safety constraints to composite position (after flip + offset + LFO).
        Clamps to stage bounds if constraint enabled, and always enforces ±50m absolute limit. */
    void applyCompositeConstraints (Position& pos, const juce::ValueTree& posSection) const;

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

    /** Update cached effect positions from parameters (positionLock held). */
    void updateEffectFeedPosition (int effectIndex);
    void updateEffectReturnPosition (int effectIndex);

    /** Find effect index from ValueTree: walk up to the <Effect> node, then the
        same count-by-type walk getEffectState uses. -1 when not under one. */
    int findEffectIndexFromTree (const juce::ValueTree& tree) const;

    /** The cone law the output and reverb-feed angular attenuations apply, on
        explicit parameters rather than a tree section: 1 within angleOn of the
        node's REAR axis, 0 within angleOff of its front axis, linear between.
        Used for the effect feeds, whose parameters are read once per recalc. */
    static float coneAttenuation (int orientationDeg, int pitchDeg, int angleOnDeg, int angleOffDeg,
                                  const Position& sourcePos, const Position& nodePos);

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
    std::vector<Position> compositeInputPositions; // [inputIndex] - Final positions (speed-limited + flip + offset + LFO)
    std::vector<Position> reverbFeedPositions;     // [reverbIndex]

    // Render-source slot map + slice geometry (protected by positionLock).
    // desc[] offsets/gains are the 50 Hz half; the slot layout itself only
    // changes with the channel-type vector (stopped-only).
    spatcore::wfs::RenderSourceMap sourceMap;

    // Per-channel backend latency for the render-latency reference hook
    // (protected by positionLock). Identically zero in Phase 0.
    std::vector<float> channelIntrinsicLatencyMs;

    std::vector<Position> reverbReturnPositions;   // [reverbIndex]
    std::vector<Position> lfoOffsets;              // [inputIndex] - LFO position offsets
    std::vector<Position> samplerCellOffsets;       // [inputIndex] - Transient sampler cell offsets
    std::vector<float> gyrophoneOffsets;           // [inputIndex] - Gyrophone rotation offsets (radians)
    std::vector<GradientMapOffsets> gradientMapOffsets;  // [inputIndex] - Gradient map parameter offsets

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

    // Effects. numEffects is the BUDGET (the feed stride), never the live count;
    // the live count is the installed render-source map's numEffectChannels.
    int numEffects = 0;
    std::vector<Position> effectFeedPositions;             // [effectIndex] base position (positionLock)
    std::vector<Position> effectReturnPositions;           // [effectIndex] base + return offset (positionLock)
    std::vector<Position> effectOtomoOffsets;              // [effectIndex] AutomOtion offset, 50 Hz (positionLock)
    std::vector<Position> effectLfoOffsets;                // [effectIndex] LFO offset, 50 Hz (positionLock)
    std::vector<Position> compositeEffectReturnPositions;  // [effectIndex] what the last recalc rendered (positionLock)
    std::vector<float> effectCommonAttenAdjustments;       // [effectIndex] the return rows' lift, reused by the feeds
                                                           // (message thread only - written and read by recalculateMatrix)

    // Source → Effect feed matrix results [sourceIndex * numEffects + effectIndex]
    std::vector<float> inputEffectDelayTimesMs;
    std::vector<float> inputEffectLevels;
    std::vector<float> inputEffectHFAttenuationDb;

    // Thread safety
    mutable juce::CriticalSection positionLock;
    mutable juce::CriticalSection matrixLock;

    // Dirty flags for lazy recalculation
    std::atomic<bool> matrixDirty { true };           // Any change requiring full recalc
    std::atomic<bool> outputsDirty { true };          // Output positions changed (affects all inputs)
    std::atomic<bool> reverbsDirty { true };          // Reverb positions changed
    std::vector<bool> inputDirtyFlags;                // Per-input dirty flags (protected by positionLock)

    // Effects dirtiness, in two grades. effectsDirty = a feed position or a feed
    // parameter changed, which re-times every source's feed row (as a reverb
    // edit re-times every input). effectReturnsDirty = only the RETURN moved
    // or a return/channel parameter changed - the return rows, the
    // return -> reverb rows and the effect -> effect feeds, but no input row;
    // this is what a running AutomOtion sets at 50 Hz, and it must stay cheap.
    std::atomic<bool> effectsDirty { true };
    std::atomic<bool> effectReturnsDirty { true };
    std::atomic<bool> effectSendsDirty { true };      // A send row changed: rebuild gains + the cycle mask
    std::atomic<bool> soloEffectsGlobal { false };
    std::atomic<uint32_t> effectCycleMask { 0 };

    // Speed of sound (m/s)
    static constexpr float speedOfSound = 343.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WFSCalculationEngine)
};
