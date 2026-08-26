#pragma once

#include <JuceHeader.h>
#include "BinauralCalculationEngine.h"
#include "../../spatcore/rt/SharedInputRingBuffer.h"
#include "../../spatcore/dsp/WFSHighShelfFilter.h"
#include "../../spatcore/rt/LockFreeRingBuffer.h"
#include "../../spatcore/rt/AudioWorkgroupCoordinator.h"
#include "../../spatcore/binaural/BinauralEngine.h"
#include "../../spatcore/binaural/HeadOrientationSource.h"
#include <vector>
#include <atomic>
#include <cmath>

/**
 * BinauralProcessor
 *
 * Thread-based processor for binaural rendering.
 * Processes inputs to a stereo binaural output pair.
 *
 * Behavior:
 * - When no inputs are soloed: ALL inputs are processed
 * - When any input is soloed: only soloed inputs are processed
 *
 * For each processed input:
 * - Applies per-input delay using circular buffer (separate L/R)
 * - Applies HF shelf filter for air absorption (separate L/R)
 * - Applies level attenuation
 * - Sums to left/right outputs
 */
class BinauralProcessor : public juce::Thread
{
public:
    explicit BinauralProcessor (BinauralCalculationEngine& calcEngine)
        : juce::Thread ("BinauralProcessor"),
          binauralCalc (calcEngine)
    {
    }

    ~BinauralProcessor() override
    {
        stopThread (1000);
    }

    /** Optional: realtime workgroup to (re)join from the worker thread (macOS). */
    void setWorkgroupCoordinator (AudioWorkgroupCoordinator* c) { workgroupCoordinator = c; }
    AudioWorkgroupCoordinator* workgroupCoordinator = nullptr;

    /**
     * Prepare the processor for playback.
     */
    void prepareToPlay (double newSampleRate, int maxBlockSize, int numInputs)
    {
        // Reconfigure only while the worker is stopped — all call sites stop the
        // thread first (MainComponent prepareToPlay/releaseResources/timerCallback,
        // handleChannelCountChange). This invariant is why sampleRate/
        // numInputChannels/currentBlockSize are deliberately non-atomic.
        jassert (! isThreadRunning());

        sampleRate = newSampleRate;
        numInputChannels = numInputs;
        currentBlockSize = maxBlockSize;

        // Maximum delay = 1 second
        delayBufferLength = (int) (sampleRate * 1.0);

        // Create per-input delay buffers for left and right
        delayBuffersL.clear();
        delayBuffersR.clear();
        writePositionsL.clear();
        writePositionsR.clear();
        hfFiltersL.clear();
        hfFiltersR.clear();
        prevParamsL.clear();
        prevParamsR.clear();
        inputBuffers.clear();

        for (int i = 0; i < numInputs; ++i)
        {
            // Delay buffers
            delayBuffersL.push_back (juce::AudioBuffer<float> (1, delayBufferLength));
            delayBuffersL.back().clear();
            delayBuffersR.push_back (juce::AudioBuffer<float> (1, delayBufferLength));
            delayBuffersR.back().clear();
            writePositionsL.push_back (0);
            writePositionsR.push_back (0);

            // HF filters
            WFSHighShelfFilter filterL, filterR;
            filterL.prepare (sampleRate);
            filterR.prepare (sampleRate);
            hfFiltersL.push_back (filterL);
            hfFiltersR.push_back (filterR);

            // Smoothed parameter state (snap on first block)
            prevParamsL.push_back (SmoothedParams());
            prevParamsR.push_back (SmoothedParams());

            // Input ring buffers (4x block size for safety margin)
            inputBuffers.push_back (std::make_unique<LockFreeRingBuffer>());
            inputBuffers.back()->setSize (maxBlockSize * 4);
        }

        // Output ring buffers
        outputBufferL = std::make_unique<LockFreeRingBuffer>();
        outputBufferR = std::make_unique<LockFreeRingBuffer>();
        outputBufferL->setSize (maxBlockSize * 4);
        outputBufferR->setSize (maxBlockSize * 4);

        // Working buffers
        inputBlock.setSize (1, maxBlockSize);
        outputBlockL.setSize (1, maxBlockSize);
        outputBlockR.setSize (1, maxBlockSize);

        // HRTF path (renderMode != 0): engine + per-source scratch, sized here
        // so the worker never allocates. Sources = inputs + reverb-node
        // returns (fixed reverb capacity so node-count changes don't force a
        // re-prepare of the engine).
        const int maxSources = numInputs + BinauralCalculationEngine::RtParams::kMaxReverbNodes;
        hrtfEngine.prepare (sampleRate, maxBlockSize, maxSources);
        hrtfInputBlock.setSize (juce::jmax (1, maxSources), maxBlockSize);
        hrtfInputPtrs.assign ((size_t) juce::jmax (1, maxSources), nullptr);
        hrtfPositions.assign ((size_t) juce::jmax (1, maxSources) * 3, 0.0f);
        hrtfSourceGains.assign ((size_t) juce::jmax (1, maxSources), 1.0f);

        prepared.store (true, std::memory_order_release);
    }

    /**
     * Release resources when stopping.
     */
    void releaseResources()
    {
        prepared.store (false, std::memory_order_release);
        stopThread (1000);
        delayBuffersL.clear();
        delayBuffersR.clear();
        hfFiltersL.clear();
        hfFiltersR.clear();
        inputBuffers.clear();
        outputBufferL.reset();
        outputBufferR.reset();
    }

    /**
     * Push input samples from audio callback (producer).
     * Call this for each input channel.
     */
    void pushInput (int inputIndex, const float* data, int numSamples)
    {
        if (inputIndex >= 0 && inputIndex < (int) inputBuffers.size())
            inputBuffers[inputIndex]->write (data, numSamples);
        notify();  // Wake binaural worker thread immediately (immune to timer coalescing)
    }

    /**
     * Pull output samples from audio callback (consumer).
     * Retrieves processed binaural stereo output.
     */
    void pullOutput (float* leftOutput, float* rightOutput, int numSamples)
    {
        int samplesReadL = outputBufferL->read (leftOutput, numSamples);
        int samplesReadR = outputBufferR->read (rightOutput, numSamples);

        // If not enough samples available, zero-pad the rest
        if (samplesReadL < numSamples)
            juce::FloatVectorOperations::clear (leftOutput + samplesReadL, numSamples - samplesReadL);
        if (samplesReadR < numSamples)
            juce::FloatVectorOperations::clear (rightOutput + samplesReadR, numSamples - samplesReadR);
    }

    /**
     * Enable or disable processing.
     */
    void setEnabled (bool enabled)
    {
        processingEnabled.store (enabled, std::memory_order_release);
    }

    /**
     * Check if processing is enabled.
     */
    bool isEnabled() const
    {
        return processingEnabled.load (std::memory_order_acquire);
    }

    /**
     * Start the processing thread.
     */
    void startProcessing()
    {
        if (!isThreadRunning())
            startRealtimeThread (juce::Thread::RealtimeOptions{}
                                     .withApproximateAudioProcessingTime (currentBlockSize, sampleRate));
    }

    /**
     * Stop the processing thread.
     */
    void stopProcessing()
    {
        stopThread (1000);
    }

    /** Set shared input buffers (reads from these instead of private ring buffers). */
    void setSharedInputBuffers(const std::vector<std::unique_ptr<SharedInputRingBuffer>>& buffers)
    {
        const juce::SpinLock::ScopedLockType lock (sharedInputsLock);
        sharedInputs.clear();
        for (auto& buf : buffers)
            sharedInputs.push_back(buf.get());
        sharedReadPositions.assign(sharedInputs.size(), 0);
        useSharedInputs = true;
        ++sharedInputsGeneration;
    }

    /** Clear shared input buffer references (fall back to pushInput mode). */
    void clearSharedInputBuffers()
    {
        const juce::SpinLock::ScopedLockType lock (sharedInputsLock);
        sharedInputs.clear();
        sharedReadPositions.clear();
        useSharedInputs = false;
        ++sharedInputsGeneration;
    }

    /** Set the reverb-return taps for the HRTF modes' spatialised reverb
        (written by the audio callback post node-trim). Message thread; safe
        while the worker runs (same lock+generation discipline as inputs). */
    void setSharedReverbBuffers (const std::vector<std::unique_ptr<SharedInputRingBuffer>>& buffers)
    {
        const juce::SpinLock::ScopedLockType lock (sharedInputsLock);
        sharedReverbs.clear();
        for (auto& buf : buffers)
            sharedReverbs.push_back (buf.get());
        sharedReverbReadPositions.assign (sharedReverbs.size(), 0);
        ++sharedInputsGeneration;
    }

    /** Notify that new input data is available in shared buffers. */
    void notifyInputAvailable()
    {
        notify();
    }

    /** HRTF engine access for the message thread (SOFA set publish/collect). */
    spatcore::binaural::BinauralEngine& getHrtfEngine() { return hrtfEngine; }

    /** Head-orientation fast path: the active tracker source, or nullptr for
        manual. Set from the message thread; the source object must outlive
        the worker (HeadTrackerManager only destroys sources at shutdown). */
    void setHeadOrientationSource (spatcore::binaural::HeadOrientationSource* src) noexcept
    {
        headOrientationSource.store (src, std::memory_order_release);
    }

    /** Block size the worker was last prepared with (message thread use). */
    int getCurrentBlockSize() const { return currentBlockSize; }

    /**
     * Reset all delay buffers and filters.
     */
    void reset()
    {
        for (auto& buf : delayBuffersL)
            buf.clear();
        for (auto& buf : delayBuffersR)
            buf.clear();
        for (auto& pos : writePositionsL)
            pos = 0;
        for (auto& pos : writePositionsR)
            pos = 0;
        for (auto& filter : hfFiltersL)
            filter.reset();
        for (auto& filter : hfFiltersR)
            filter.reset();
        for (auto& p : prevParamsL)
            p.initialized = false;
        for (auto& p : prevParamsR)
            p.initialized = false;
        for (auto& buf : inputBuffers)
            buf->reset();
        if (outputBufferL) outputBufferL->reset();
        if (outputBufferR) outputBufferR->reset();
        hrtfEngine.reset();
        lastOrientationSource = nullptr;
        haveLastGood = false;
    }

    /**
     * Update for changed input channel count.
     */
    void setNumInputChannels (int numInputs)
    {
        if (numInputs != numInputChannels && sampleRate > 0)
        {
            bool wasRunning = isThreadRunning();
            if (wasRunning) stopThread (1000);
            prepareToPlay (sampleRate, currentBlockSize, numInputs);
            if (wasRunning) startRealtimeThread (juce::Thread::RealtimeOptions{}
                                                     .withApproximateAudioProcessingTime (currentBlockSize, sampleRate));
        }
    }

private:
    // Per-input smoothed parameter state for interpolation between blocks
    struct SmoothedParams
    {
        float delayMs = 0.0f;
        float level = 0.0f;
        float hfDb = 0.0f;
        bool initialized = false;
    };

    /**
     * Worker thread main loop.
     */
    void run() override
    {
        // Reusable snapshot storage — allocated once, then just refilled each
        // batch under sharedInputsLock so we never allocate in the hot path
        // after the first few iterations.
        std::vector<SharedInputRingBuffer*> sharedInputsSnap;
        std::vector<int> readPositionsSnap;
        std::vector<SharedInputRingBuffer*> sharedReverbsSnap;
        std::vector<int> reverbReadPositionsSnap;

        // Audio workgroup membership: token lives on (and is destroyed on) this thread.
        juce::WorkgroupToken wgToken;
        uint32_t wgSeenGeneration = 0;

        while (!threadShouldExit())
        {
            // The prepared guard is what makes an out-of-order enable merely
            // silent instead of fatal: enabled-but-never-prepared used to run
            // the block loop into null output rings (field crash: binaural
            // enabled while no audio device was open).
            if (processingEnabled.load (std::memory_order_acquire)
                && prepared.load (std::memory_order_acquire))
            {
                if (workgroupCoordinator != nullptr)
                    workgroupCoordinator->joinIfChanged (wgToken, wgSeenGeneration);

                // Snapshot the shared-input triplet under the lock so we never
                // read a vector mid-reallocation.
                bool useSharedSnap;
                uint32_t snapshotGeneration;
                {
                    const juce::SpinLock::ScopedLockType lock (sharedInputsLock);
                    useSharedSnap = useSharedInputs;
                    sharedInputsSnap = sharedInputs;
                    readPositionsSnap = sharedReadPositions;
                    sharedReverbsSnap = sharedReverbs;
                    reverbReadPositionsSnap = sharedReverbReadPositions;
                    snapshotGeneration = sharedInputsGeneration;
                }

                // Check if we have enough input data to process a block
                bool hasData = true;

                if (useSharedSnap)
                {
                    for (int i = 0; i < numInputChannels && i < (int)sharedInputsSnap.size() && hasData; ++i)
                    {
                        if (sharedInputsSnap[i]->getAvailableAt(readPositionsSnap[i]) < currentBlockSize)
                            hasData = false;
                    }
                }
                else
                {
                    for (int i = 0; i < numInputChannels && hasData; ++i)
                    {
                        if (inputBuffers[i]->getAvailableData() < currentBlockSize)
                            hasData = false;
                    }
                }

                if (hasData)
                {
                    processBlock (useSharedSnap, sharedInputsSnap, readPositionsSnap,
                                  sharedReverbsSnap, reverbReadPositionsSnap);

                    // Write back advanced read positions — but only if the
                    // writer didn't reconfigure us mid-batch. Reconfigure
                    // already reset positions to 0, so discarding our local
                    // updates is correct in that case.
                    if (useSharedSnap)
                    {
                        const juce::SpinLock::ScopedLockType lock (sharedInputsLock);
                        if (sharedInputsGeneration == snapshotGeneration)
                        {
                            sharedReadPositions = readPositionsSnap;
                            sharedReverbReadPositions = reverbReadPositionsSnap;
                        }
                    }
                }
                else
                {
                    // Wait a short time for more data
                    wait (1);
                }
            }
            else
            {
                // Not enabled, wait longer
                wait (10);
            }
        }
    }

    /**
     * Process one block of audio.
     * The shared-input state is passed in as a local snapshot so the hot path
     * never touches the (racy-to-mutate) member vectors directly.
     */
    void processBlock (bool useSharedSnap,
                       const std::vector<SharedInputRingBuffer*>& sharedInputsSnap,
                       std::vector<int>& readPositionsSnap,
                       const std::vector<SharedInputRingBuffer*>& sharedReverbsSnap,
                       std::vector<int>& reverbReadPositionsSnap)
    {
        int numSamples = currentBlockSize;

        // One snapshot copy per block — the RT thread's only parameter source.
        // Never read the ValueTree from here (RT-safety: no locks on the tree, no allocation).
        const auto rt = binauralCalc.getRtParams();

        // HRTF render modes take their own path; the legacy ORTF code below is
        // otherwise untouched, so mode 0 keeps its pre-HRTF behaviour.
        //
        // That null is now exact only at Orbit 0 and 180. The capsule facings
        // in BinauralCalculationEngine::recalculatePositions counter-rotated
        // against the seat, which was a plain sign error, not a behaviour worth
        // preserving: at Orbit ±90 the pair aimed away from the stage and the
        // keystone pattern took ~8 dB off it, lopsidedly. Corrected there.
        // (The legacy path never consumes the reverb taps; the HRTF path
        // resyncs their cursors when it takes over.)
        if (rt.renderMode != 0)
        {
            processBlockHrtf (rt, useSharedSnap, sharedInputsSnap, readPositionsSnap,
                              sharedReverbsSnap, reverbReadPositionsSnap);
            return;
        }

        // Clear output accumulators
        outputBlockL.clear();
        outputBlockR.clear();

        float* outL = outputBlockL.getWritePointer (0);
        float* outR = outputBlockR.getWritePointer (0);

        // Check if any inputs are soloed
        bool anySoloed = rt.numSoloed > 0;

        // Process each render source. Solo is per CHANNEL: a stereo slice
        // follows its owning channel's solo state. Slice gain is 1 for mono
        // sources, 0 for inactive slice slots (whose buffers are silent
        // anyway) — the multiply below is skipped at exactly 1, keeping the
        // mono-only path byte-identical to pre-stereo builds.
        for (int inputIdx = 0; inputIdx < numInputChannels; ++inputIdx)
        {
            const float sourceGain = binauralCalc.getRenderSourceGain (inputIdx);

            // Skip if soloed mode and this source's channel isn't soloed
            if (anySoloed && !rt.isSoloed (binauralCalc.getOwningInputChannel (inputIdx)))
            {
                // Still need to consume input data to keep buffers in sync
                if (useSharedSnap && inputIdx < (int)sharedInputsSnap.size())
                    sharedInputsSnap[inputIdx]->readWithPosition(readPositionsSnap[inputIdx], inputBlock.getWritePointer(0), numSamples);
                else
                    inputBuffers[inputIdx]->read (inputBlock.getWritePointer (0), numSamples);
                continue;
            }

            // Read input from shared buffers or private ring buffers
            int samplesRead;
            if (useSharedSnap && inputIdx < (int)sharedInputsSnap.size())
                samplesRead = sharedInputsSnap[inputIdx]->readWithPosition(readPositionsSnap[inputIdx], inputBlock.getWritePointer(0), numSamples);
            else
                samplesRead = inputBuffers[inputIdx]->read (inputBlock.getWritePointer (0), numSamples);
            if (samplesRead == 0)
                continue;

            // Get binaural parameters for this input (tree-free, snapshot-driven)
            auto binauralPair = binauralCalc.calculate (inputIdx, rt);

            if (sourceGain != 1.0f)
            {
                binauralPair.left.level  *= sourceGain;
                binauralPair.right.level *= sourceGain;
            }

            const float* inputData = inputBlock.getReadPointer (0);

            // Process left channel
            processInputToChannel (inputIdx, inputData, samplesRead,
                                   binauralPair.left,
                                   delayBuffersL[inputIdx],
                                   writePositionsL[inputIdx],
                                   hfFiltersL[inputIdx],
                                   outL,
                                   prevParamsL[inputIdx]);

            // Process right channel
            processInputToChannel (inputIdx, inputData, samplesRead,
                                   binauralPair.right,
                                   delayBuffersR[inputIdx],
                                   writePositionsR[inputIdx],
                                   hfFiltersR[inputIdx],
                                   outR,
                                   prevParamsR[inputIdx]);
        }

        // Write to output ring buffers
        outputBufferL->write (outL, numSamples);
        outputBufferR->write (outR, numSamples);
    }

    /**
     * HRTF render path (Structural / SOFA). Consumes exactly one block from
     * every input ring (same cadence as the legacy path, so switching modes
     * never desyncs the shared read cursors), builds the listener pose, and
     * hands the block to the spatcore engine. RT-safe: all scratch was sized
     * in prepareToPlay().
     */
    void processBlockHrtf (const BinauralCalculationEngine::RtParams& rt,
                           bool useSharedSnap,
                           const std::vector<SharedInputRingBuffer*>& sharedInputsSnap,
                           std::vector<int>& readPositionsSnap,
                           const std::vector<SharedInputRingBuffer*>& sharedReverbsSnap,
                           std::vector<int>& reverbReadPositionsSnap)
    {
        const int numSamples = currentBlockSize;

        outputBlockL.clear();
        outputBlockR.clear();
        float* outL = outputBlockL.getWritePointer (0);
        float* outR = outputBlockR.getWritePointer (0);

        const bool anySoloed = rt.numSoloed > 0;

        // Read every input ring (always consume — cursor discipline), fill
        // per-source pointers/positions. Solo-gated inputs pass nullptr.
        for (int i = 0; i < numInputChannels; ++i)
        {
            float* dest = hrtfInputBlock.getWritePointer (i);
            int samplesRead;
            if (useSharedSnap && i < (int) sharedInputsSnap.size())
                samplesRead = sharedInputsSnap[(size_t) i]->readWithPosition (readPositionsSnap[(size_t) i], dest, numSamples);
            else
                samplesRead = inputBuffers[(size_t) i]->read (dest, numSamples);

            // Solo is per CHANNEL (a stereo slice follows its owning channel);
            // slice gain mutes inactive slots and carries Phase-1 confidence
            // trims. Mono sources: owning == i and gain == 1, unchanged.
            const bool active = samplesRead > 0
                             && ! (anySoloed && ! rt.isSoloed (binauralCalc.getOwningInputChannel (i)));
            if (samplesRead > 0 && samplesRead < numSamples)
                juce::FloatVectorOperations::clear (dest + samplesRead, numSamples - samplesRead);

            hrtfInputPtrs[(size_t) i] = active ? dest : nullptr;
            hrtfSourceGains[(size_t) i] = binauralCalc.getRenderSourceGain (i);

            const auto pos = binauralCalc.getInputPosition (i);
            hrtfPositions[(size_t) i * 3 + 0] = pos.x;
            hrtfPositions[(size_t) i * 3 + 1] = pos.y;
            hrtfPositions[(size_t) i * 3 + 2] = pos.z;
        }

        // Spatialised reverb: node returns become extra sources at their
        // return positions — studio-preview only (any solo mutes the tap,
        // since nodes carry a mix of every input), scaled by the headphone
        // reverb balance. Best-effort reads: a ring that can't supply a full
        // block (engine just started, WFS stopped, post-mute) is silent this
        // block, and a stale cursor (mode just switched) resyncs to the
        // freshest block instead of replaying old audio.
        const int numReverb = juce::jmin (rt.numReverbNodes, (int) sharedReverbsSnap.size());
        const int totalSources = numInputChannels
                               + BinauralCalculationEngine::RtParams::kMaxReverbNodes;
        for (int r = 0; r < BinauralCalculationEngine::RtParams::kMaxReverbNodes; ++r)
        {
            const int srcIdx = numInputChannels + r;
            if (srcIdx >= (int) hrtfInputPtrs.size())
                break;
            hrtfInputPtrs[(size_t) srcIdx] = nullptr;

            if (r >= numReverb || anySoloed)
                continue;

            auto* ring = sharedReverbsSnap[(size_t) r];
            if (ring == nullptr)
                continue;

            int& cursor = reverbReadPositionsSnap[(size_t) r];
            int available = ring->getAvailableAt (cursor);
            if (available > 2 * numSamples)
            {
                // Stale cursor: jump to the freshest full block.
                cursor = (cursor + (available - numSamples)) % ring->getBufferSize();
                available = numSamples;
            }
            if (available < numSamples)
                continue;

            float* dest = hrtfInputBlock.getWritePointer (srcIdx);
            const int got = ring->readWithPosition (cursor, dest, numSamples);
            if (got < numSamples)
                juce::FloatVectorOperations::clear (dest + got, numSamples - got);

            hrtfInputPtrs[(size_t) srcIdx] = dest;
            hrtfSourceGains[(size_t) srcIdx] = rt.reverbAttenLinear;
            hrtfPositions[(size_t) srcIdx * 3 + 0] = rt.reverbPos[r][0];
            hrtfPositions[(size_t) srcIdx * 3 + 1] = rt.reverbPos[r][1];
            hrtfPositions[(size_t) srcIdx * 3 + 2] = rt.reverbPos[r][2];
        }

        // Listener pose: damped position from the 50 Hz snapshot; orientation
        // offsets from the FAST path when a tracker is active (read fresh
        // every block, bypassing the damped pipeline entirely — the engine's
        // per-block slew and delay smoothing absorb the steps), else from the
        // manual parameters in the snapshot.
        //
        // A SELECTED tracker that goes momentarily quiet (webcam blinks, face
        // briefly lost, a dropped USB frame) holds its last good attitude
        // instead of reverting to the manual angles. Reverting was a step in
        // the wrong DIRECTION, not a click — the smoothing below absorbs the
        // step either way — but with a non-zero manual yaw it swung the whole
        // room to a different heading and back every time tracking hiccuped.
        // The manual angles are the fallback for "no tracker selected", which
        // is what they mean; they are not a stand-in for a tracker that is
        // simply between samples.
        // Nothing non-finite may reach the renderers. A NaN attitude builds a
        // non-orthonormal rotation, and from there NaN spreads into the ITD
        // delay lines (where the read index is derived from the delay — a
        // float→int conversion of NaN is undefined), the shadow/notch biquad
        // state and the SOFA convolution history, none of which recover on
        // their own. The RT stage does not get to trust its sources: the
        // tracker hands over raw plugin data, and the manual angles come from
        // a project file (juce::jlimit passes NaN straight through, since
        // every comparison with NaN is false).
        spatcore::binaural::ListenerPose pose;
        pose.x = rt.listenerX;
        pose.y = rt.listenerY;
        pose.z = rt.listenerZ;
        if (! (std::isfinite (pose.x) && std::isfinite (pose.y) && std::isfinite (pose.z)))
            pose.x = pose.y = pose.z = 0.0f;
        {
            float yaw = rt.manualYawRad, pitch = rt.manualPitchRad, roll = rt.manualRollRad;

            auto* src = headOrientationSource.load (std::memory_order_acquire);
            if (src != lastOrientationSource)
            {
                // Selection changed (including to manual): whatever we were
                // holding belonged to the previous source and means nothing now.
                lastOrientationSource = src;
                haveLastGood = false;
            }

            if (src != nullptr)
            {
                const auto tracked = src->getOrientation();
                if (tracked.valid && spatcore::binaural::isFiniteAttitude (tracked))
                {
                    yaw = tracked.yawRad;
                    pitch = tracked.pitchRad;
                    roll = tracked.rollRad;
                    lastGoodYaw = yaw;
                    lastGoodPitch = pitch;
                    lastGoodRoll = roll;
                    haveLastGood = true;
                }
                else if (haveLastGood)
                {
                    yaw = lastGoodYaw;
                    pitch = lastGoodPitch;
                    roll = lastGoodRoll;
                }
                // else: tracker selected but has never delivered a good sample
                // (just selected, camera still opening) — the manual angles are
                // the only thing we have, and facing the origin is the right
                // place to start from.
            }

            // Manual angles (or a source that bypassed the publish guard):
            // fall back to facing the origin rather than rendering NaN.
            if (! (std::isfinite (yaw) && std::isfinite (pitch) && std::isfinite (roll)))
                yaw = pitch = roll = 0.0f;

            float offset[9];
            spatcore::binaural::headframe::yawPitchRollToMatrix (yaw, pitch, roll, offset);
            spatcore::binaural::headframe::composeWithBaseline (
                std::isfinite (rt.baselineAngleRad) ? rt.baselineAngleRad : 0.0f, offset, pose.R);
        }

        hrtfEngine.setMode (static_cast<spatcore::binaural::RenderMode> (rt.renderMode));
        hrtfEngine.setHeadRadius (rt.headRadius);
        hrtfEngine.processBlock (pose,
                                 reinterpret_cast<const float (*)[3]> (hrtfPositions.data()),
                                 hrtfInputPtrs.data(), hrtfSourceGains.data(), rt.delayOffsetMs,
                                 outL, outR, totalSources, numSamples);

        // Master trim at the sum (per-input in the legacy path; here the
        // engine output is trimmed once).
        if (rt.attenLinear != 1.0f)
        {
            juce::FloatVectorOperations::multiply (outL, rt.attenLinear, numSamples);
            juce::FloatVectorOperations::multiply (outR, rt.attenLinear, numSamples);
        }

        outputBufferL->write (outL, numSamples);
        outputBufferR->write (outR, numSamples);
    }

    /**
     * Process one input to one output channel (left or right).
     * Uses fractional delay with linear interpolation and per-sample
     * parameter interpolation to avoid graininess on fast position changes.
     */
    void processInputToChannel (int inputIdx,
                                const float* inputData,
                                int numSamples,
                                const BinauralCalculationEngine::BinauralOutput& params,
                                juce::AudioBuffer<float>& delayBuffer,
                                int& writePos,
                                WFSHighShelfFilter& hfFilter,
                                float* output,
                                SmoothedParams& prevParams)
    {
        juce::ignoreUnused (inputIdx);
        float* delayData = delayBuffer.getWritePointer (0);

        // First block after init: snap to current values (no interpolation from zero)
        if (!prevParams.initialized)
        {
            prevParams.delayMs = params.delayMs;
            prevParams.level = params.level;
            prevParams.hfDb = params.hfAttenuationDb;
            prevParams.initialized = true;
        }

        // Per-sample interpolation endpoints
        float startDelayMs = prevParams.delayMs;
        float startLevel = prevParams.level;
        float endDelayMs = params.delayMs;
        float endLevel = params.level;
        float invNumSamples = 1.0f / (float) numSamples;

        // Update prev for next block
        prevParams.delayMs = params.delayMs;
        prevParams.level = params.level;
        prevParams.hfDb = params.hfAttenuationDb;

        // Set HF filter gain (filter state provides inherent smoothing)
        hfFilter.setGainDb (params.hfAttenuationDb);

        float msToSamples = (float) (sampleRate / 1000.0);
        float maxDelay = (float) (delayBufferLength - 2);

        // Process each sample with interpolated parameters
        for (int i = 0; i < numSamples; ++i)
        {
            // Write input to delay buffer
            delayData[writePos] = inputData[i];

            // Interpolate delay and level across the block
            float t = (float) i * invNumSamples;
            float currentDelayMs = startDelayMs + (endDelayMs - startDelayMs) * t;
            float currentLevel = startLevel + (endLevel - startLevel) * t;

            // Fractional delay in samples
            float delaySamples = currentDelayMs * msToSamples;
            if (delaySamples < 0.0f) delaySamples = 0.0f;
            if (delaySamples > maxDelay) delaySamples = maxDelay;

            // Fractional read position with linear interpolation
            float exactReadPos = (float) writePos - delaySamples;
            if (exactReadPos < 0.0f)
                exactReadPos += (float) delayBufferLength;

            int readPos1 = (int) exactReadPos;
            if (readPos1 >= delayBufferLength) readPos1 -= delayBufferLength;
            int readPos2 = (readPos1 + 1) % delayBufferLength;
            float fraction = exactReadPos - std::floor (exactReadPos);

            float delayedSample = delayData[readPos1] + fraction * (delayData[readPos2] - delayData[readPos1]);

            // Apply HF filter
            float filteredSample = hfFilter.processSample (delayedSample);

            // Apply interpolated level and sum to output
            output[i] += filteredSample * currentLevel;

            // Advance write position
            writePos = (writePos + 1) % delayBufferLength;
        }
    }

    BinauralCalculationEngine& binauralCalc;

    // Deliberately non-atomic: written only in prepareToPlay(), which asserts the
    // worker thread is stopped (thread join/start provides the happens-before edge).
    double sampleRate = 48000.0;
    int numInputChannels = 0;
    int currentBlockSize = 512;
    int delayBufferLength = 0;

    std::atomic<bool> processingEnabled {false};
    std::atomic<bool> prepared {false};   // set by prepareToPlay, cleared by releaseResources

    // Shared input buffers (read from these when available, bypasses pushInput).
    // The (sharedInputs, sharedReadPositions, useSharedInputs) triplet is published
    // by setSharedInputBuffers() / clearSharedInputBuffers() from the message
    // thread and consumed by run() on the worker thread. sharedInputsLock
    // serialises publication so the worker never observes the vectors mid-mutation
    // (clear() + push_back() can reallocate the backing pointer array). The
    // worker snapshots the triplet once per batch and writes back advanced
    // positions under the lock only if sharedInputsGeneration hasn't changed
    // (i.e. no reconfigure happened during the batch).
    std::vector<SharedInputRingBuffer*> sharedInputs;
    std::vector<int> sharedReadPositions;
    bool useSharedInputs = false;
    juce::SpinLock sharedInputsLock;
    uint32_t sharedInputsGeneration = 0;

    // Lock-free ring buffers for input (fallback when shared buffers aren't set)
    std::vector<std::unique_ptr<LockFreeRingBuffer>> inputBuffers;

    // Lock-free ring buffers for output (L/R stereo)
    std::unique_ptr<LockFreeRingBuffer> outputBufferL;
    std::unique_ptr<LockFreeRingBuffer> outputBufferR;

    // Per-input delay buffers (separate for left and right)
    std::vector<juce::AudioBuffer<float>> delayBuffersL;
    std::vector<juce::AudioBuffer<float>> delayBuffersR;
    std::vector<int> writePositionsL;
    std::vector<int> writePositionsR;

    // Per-input HF shelf filters
    std::vector<WFSHighShelfFilter> hfFiltersL;
    std::vector<WFSHighShelfFilter> hfFiltersR;

    std::vector<SmoothedParams> prevParamsL;
    std::vector<SmoothedParams> prevParamsR;

    // Working buffers
    juce::AudioBuffer<float> inputBlock;
    juce::AudioBuffer<float> outputBlockL;
    juce::AudioBuffer<float> outputBlockR;

    // HRTF path (renderMode != 0): spatcore engine + per-source scratch.
    // hrtfPositions is a flat [numInputs][3] world-frame array.
    spatcore::binaural::BinauralEngine hrtfEngine;
    std::atomic<spatcore::binaural::HeadOrientationSource*> headOrientationSource { nullptr };

    // Last attitude a SELECTED tracker actually delivered, so a momentary
    // dropout holds the heading instead of reverting to the manual angles.
    // Worker-thread-private: written and read only by processBlockHrtf (and
    // cleared by reset(), which runs with the worker stopped). lastOrientationSource
    // is compared, never dereferenced — it only detects a selection change.
    const spatcore::binaural::HeadOrientationSource* lastOrientationSource = nullptr;
    float lastGoodYaw = 0.0f, lastGoodPitch = 0.0f, lastGoodRoll = 0.0f;
    bool haveLastGood = false;
    juce::AudioBuffer<float> hrtfInputBlock;
    std::vector<const float*> hrtfInputPtrs;
    std::vector<float> hrtfPositions;
    std::vector<float> hrtfSourceGains;

    // Reverb-return taps (HRTF modes' spatialised reverb) — same publish
    // discipline as sharedInputs above.
    std::vector<SharedInputRingBuffer*> sharedReverbs;
    std::vector<int> sharedReverbReadPositions;
};
