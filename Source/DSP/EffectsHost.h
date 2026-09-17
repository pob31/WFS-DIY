#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "../WFSLogger.h"
#include "WFSCalculationEngine.h"
#include "../../spatcore/effects/EffectsEngine.h"
#include "../../spatcore/effects/EffectParams.h"
#include "../../spatcore/effects/EffectsTypes.h"
#include "../../spatcore/rt/SharedInputRingBuffer.h"
#include "../../spatcore/rt/AudioWorkgroupCoordinator.h"

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

//==============================================================================
/**
    The app's side of the effects engine: everything that stands between the
    parameter tree, the calculation engine and spatcore's EffectsEngine.

    THE SHAPE. One object owns the engine and does the four jobs the reverb
    feed thread's wiring spreads across MainComponent:

      - prepare / release, with the config cooked from Config/EffectsGlobal and
        the render-source layout (the engine must be told where the return
        rows sit and how wide the feed matrix is);
      - the pop: every return into its render-source row of the input buffer,
        at the top of the callback, before the rows are written to the rings;
      - the cook: the 174 per-channel properties transcribed into one
        EffectChannelParams and published, coalesced per 50 Hz tick per channel
        with a revision the chain diffs on;
      - telemetry, read from the core for the meters, the log and the trace.

    THREADS. prepare(), release(), publishDirty() and setFeedMatrices() are
    message-thread calls; pullReturns(), notifyInputAvailable() and setMuted()
    are audio-thread calls and never block (the engine's pop is a try-lock
    that degrades to silence). The cook reads the ValueTree, so it is message
    thread only - the realtime side receives a plain POD through the engine's
    triple buffer and never touches a tree.

    LIFETIME. The engine caches raw pointers into the ring buffers it is given
    and into the calculation engine's matrices. release() must therefore run
    BEFORE the rings are destroyed at every site that clears them, and this
    object must be declared after the calculation engine and the rings in its
    owner so it is destroyed first. It never owns the rings.

    WHAT IT DOES NOT DO. It does not decide WHEN to prepare - the owner does,
    where it rebuilds the rings - and it takes no action on the cycle mask or
    the loop guard beyond forwarding the switch: the user owns the loops.
*/
class EffectsHost : private juce::ValueTree::Listener
{
public:
    using Engine = spatcore::effects::EffectsEngine;
    using Core = spatcore::effects::EffectsEngineCore;
    using Config = spatcore::effects::EffectsEngine::Config;
    using ChannelParams = spatcore::effects::EffectChannelParams;
    using ChainOrder = spatcore::effects::ChainOrder;

    static constexpr int kMaxEffects = WFSParameterDefaults::maxEffectChannels;

    explicit EffectsHost (WFSValueTreeState& state)
        : valueTreeState (state)
    {
        lastGoodOrder.fill (spatcore::effects::kDefaultOrder);
        valueTreeState.addListener (this);
    }

    ~EffectsHost() override
    {
        release();
        valueTreeState.removeListener (this);
    }

    /** macOS realtime workgroup; set BEFORE prepare(), which creates the
        worker pool that joins it. */
    void setWorkgroupCoordinator (AudioWorkgroupCoordinator* coordinator)
    {
        engine.setWorkgroupCoordinator (coordinator);
    }

    //==========================================================================
    // Lifecycle (message thread, engine stopped)
    //==========================================================================

    /** The engine config for one session, cooked from Config/EffectsGlobal
        and the render-source layout. Pure: reads the tree, allocates nothing.

        matrixStride is the effects BUDGET, never the live count - the
        calculation engine strides its feed matrices by the budget, and a
        stride that disagreed would make the engine read the wrong cells.
        firstEffectSourceRow is where the returns sit in the ring order, which
        the loop guard needs to tell the input rows from the effect rows. */
    static Config buildConfig (WFSValueTreeState& vts, double sampleRate, int blockSize,
                               int numSources, int numEffects, int firstEffectSlot)
    {
        using namespace WFSParameterIDs;
        using namespace WFSParameterDefaults;

        Config config;
        config.sampleRate = sampleRate;
        config.blockSize = blockSize;
        config.numSources = numSources;
        config.numEffects = numEffects;
        config.matrixStride = maxEffectChannels;
        config.firstEffectSourceRow = firstEffectSlot;

        auto globals = vts.getEffectsGlobalSection();
        auto readInt = [&globals] (const juce::Identifier& key, int fallback)
        {
            return globals.isValid() && globals.hasProperty (key)
                       ? static_cast<int> (globals.getProperty (key)) : fallback;
        };
        auto readFloat = [&globals] (const juce::Identifier& key, float fallback)
        {
            return globals.isValid() && globals.hasProperty (key)
                       ? static_cast<float> (static_cast<double> (globals.getProperty (key))) : fallback;
        };

        config.workerThreads = juce::jlimit (effectsGlobalWorkerThreadsMin, effectsGlobalWorkerThreadsMax,
                                             readInt (effectsGlobalWorkerThreads, effectsGlobalWorkerThreadsDefault));

        // 0 = auto in the tree, -1 = auto in the engine
        const int cushion = readInt (effectsGlobalReturnCushion, effectsGlobalReturnCushionDefault);
        config.returnCushionBlocks = cushion <= 0 ? -1
                                                  : juce::jlimit (1, effectsGlobalReturnCushionMax, cushion);

        config.loopGuardEnabled = readInt (effectsGlobalLoopGuard, effectsGlobalLoopGuardDefault) != 0;
        config.loopGuardCeilingDb = juce::jlimit (effectsGlobalLoopGuardCeilingMin, effectsGlobalLoopGuardCeilingMax,
                                                  readFloat (effectsGlobalLoopGuardCeiling, effectsGlobalLoopGuardCeilingDefault));
        config.maxEffectDelaySeconds = static_cast<double> (juce::jlimit (effectsGlobalMaxDelaySecondsMin,
                                                                          effectsGlobalMaxDelaySecondsMax,
                                                                          readInt (effectsGlobalMaxDelaySeconds,
                                                                                   effectsGlobalMaxDelaySecondsDefault)));

        // One second of feed history: the longest delay the calculation engine
        // can publish (stage diagonal + Haas + trims) is well under it, and the
        // history must exceed the longest delay by a block. A geometry-derived
        // cap is a follow-up; the cost is 4 bytes x sr x sources.
        config.maxFeedDelaySeconds = 1.0;
        config.noiseKeyBase = 1;
        return config;
    }

    /** Allocates and prepares the engine for one session. Returns false (and
        logs why) when there is nothing to run or the engine refuses - a ring
        shorter than two blocks, for instance. Does NOT start the driver: the
        owner does, with the priority it chooses, exactly as for the reverb
        feed thread. */
    bool prepare (double sampleRate, int blockSize, int numRenderSources, int firstEffectSlot, int numEffects,
                  const std::vector<std::unique_ptr<SharedInputRingBuffer>>& rings)
    {
        release();

        numEffects = juce::jlimit (0, kMaxEffects, numEffects);
        if (numEffects <= 0 || firstEffectSlot < 0 || numRenderSources <= 0
            || firstEffectSlot + numEffects > numRenderSources
            || numRenderSources > static_cast<int> (rings.size()))
        {
            WFSLogger::getInstance().logWarning ("Effects engine not prepared: effects=" + juce::String (numEffects)
                                                 + " sources=" + juce::String (numRenderSources)
                                                 + " firstSlot=" + juce::String (firstEffectSlot)
                                                 + " rings=" + juce::String ((int) rings.size()));
            return false;
        }

        const auto config = buildConfig (valueTreeState, sampleRate, blockSize, numRenderSources, numEffects, firstEffectSlot);

        if (! engine.prepare (config, rings))
        {
            WFSLogger::getInstance().logWarning ("Effects engine refused its configuration: sr=" + juce::String (sampleRate)
                                                 + " block=" + juce::String (blockSize)
                                                 + " sources=" + juce::String (numRenderSources)
                                                 + " effects=" + juce::String (numEffects));
            return false;
        }

        preparedEffects = numEffects;
        firstSlot = firstEffectSlot;
        preparedSources = numRenderSources;

        const auto& core = engine.getCore();
        WFSLogger::getInstance().logInfo ("Effects engine prepared: " + juce::String (numEffects) + " effects, "
                                          + juce::String (numRenderSources) + " sources (returns from slot "
                                          + juce::String (firstEffectSlot) + "), block " + juce::String (blockSize)
                                          + " @ " + juce::String (sampleRate, 0) + " Hz, cushion "
                                          + juce::String (core.getReturnCushionBlocks()) + " block(s), workers "
                                          + juce::String (core.getNumWorkers()));

        // Every live channel publishes on the first tick; until then the
        // engine runs its defaults (every module bypassed, transparent)
        markAllDirty();
        return true;
    }

    void startRealtimeThread (const juce::Thread::RealtimeOptions& options)
    {
        if (isPrepared())
            engine.startRealtimeThread (options);
    }

    /** Joins the driver and frees. Idempotent. Must run BEFORE the rings the
        engine was given are destroyed. The audio thread may still be inside
        pullReturns() and gets silence. */
    void release()
    {
        engine.release();
        preparedEffects = 0;
        preparedSources = 0;
        firstSlot = -1;
    }

    bool isPrepared() const noexcept { return preparedEffects > 0; }
    bool isReady() const noexcept { return engine.isReady(); }
    int getPreparedEffectCount() const noexcept { return preparedEffects; }
    int getPreparedSourceCount() const noexcept { return preparedSources; }
    int getFirstEffectSlot() const noexcept { return firstSlot; }

    //==========================================================================
    // Audio thread
    //==========================================================================

    /** Pop every return into its render-source row [firstSlot + fx] of the
        buffer, silence when the engine is not ready or a return is late. Call
        at the top of the callback, after the input patch (which clears every
        row) and BEFORE the rows are written to the rings - the engine's own
        fx-to-fx feed reads the returns from block n through those rings. */
    void pullReturns (juce::AudioBuffer<float>& buffer, int startSample, int numSamples, int liveEffects) noexcept
    {
        if (firstSlot < 0 || ! engine.isReady())
            return;

        const int count = juce::jmin (preparedEffects, liveEffects);
        for (int fx = 0; fx < count; ++fx)
        {
            const int slot = firstSlot + fx;
            if (slot >= buffer.getNumChannels())
                break;

            engine.pullReturn (fx, buffer.getWritePointer (slot, startSample), numSamples);
        }
    }

    void notifyInputAvailable() noexcept { engine.notifyInputAvailable(); }
    void setMuted (bool muted) noexcept  { engine.setMuted (muted); }

    //==========================================================================
    // Message thread, 50 Hz
    //==========================================================================

    /** Hand the engine the calculation engine's feed triplet. Six scalars
        under the engine's spin lock; call when a recalc ran, and once after
        prepare(). The pointers stay valid for the life of the calculation
        engine (its vectors are never reallocated). */
    void setFeedMatrices (const WFSCalculationEngine& calc, int numRenderSources) noexcept
    {
        if (! isPrepared())
            return;

        engine.setFeedMatrices (calc.getInputEffectDelayTimesMs(),
                                calc.getInputEffectLevels(),
                                calc.getInputEffectHFAttenuationDb(),
                                calc.getNumEffects(),
                                juce::jmin (numRenderSources, preparedSources),
                                preparedEffects);
    }

    /** Cook and publish every channel that changed since the last call: one
        cook and one publish per channel per tick however many properties
        moved (an undo of fifty properties costs one). */
    void publishDirty()
    {
        JUCE_ASSERT_MESSAGE_THREAD

        if (! isPrepared())
            return;

        const uint32_t mask = dirtyMask.exchange (0);
        if (mask == 0)
            return;

        for (int fx = 0; fx < preparedEffects; ++fx)
        {
            if ((mask & (1u << fx)) == 0)
                continue;

            bool orderOk = true;
            ChannelParams p = cookChannel (valueTreeState, fx, lastGoodOrder[static_cast<size_t> (fx)], orderOk);

            if (! orderOk)
            {
                // Once per distinct bad string: a rejected order keeps the
                // last good one running, and the log should say so once
                const auto text = valueTreeState.getEffectChainSection (fx)
                                      .getProperty (WFSParameterIDs::effectChainOrder).toString();
                if (text != lastLoggedBadOrder[static_cast<size_t> (fx)])
                {
                    lastLoggedBadOrder[static_cast<size_t> (fx)] = text;
                    WFSLogger::getInstance().logWarning ("Effect " + juce::String (fx + 1)
                                                         + ": chain order '" + text
                                                         + "' is not a permutation of the eleven slots - keeping the previous order");
                }
            }

            // Starts at 1 so the first publish differs from the engine's 0
            p.revision = ++revision[static_cast<size_t> (fx)];
            engine.publishChannelParams (fx, p);
        }
    }

    void markAllDirty()
    {
        dirtyMask.store (preparedEffects >= 32 ? 0xFFFFFFFFu : ((1u << preparedEffects) - 1u));
    }

    /** Emergency Clear: at the next batch boundary every chain, the feed
        delay lines and the return ring(s) of the target are silenced and
        reset. fx = -1 clears everything (and memsets one delay line per
        source, so a block or two may drop - it is an emergency button). */
    void requestClear (int fx = -1) noexcept { engine.requestClear (fx); }

    void setLoopGuardEnabled (bool enabled) noexcept
    {
        if (isPrepared())
            engine.getCore().setLoopGuardEnabled (enabled);
    }

    /** The core, for meters and diagnostics; nullptr until prepared. */
    const Core* getCore() const noexcept { return isPrepared() ? &engine.getCore() : nullptr; }

    /** One line for the engine and one per live effect, for the trace log. */
    juce::String describeTelemetry() const
    {
        if (! isPrepared())
            return "effects: not prepared";

        const auto& core = engine.getCore();
        auto dB = [] (float linear) { return juce::String (20.0f * std::log10 (juce::jmax (linear, 1.0e-6f)), 1); };

        juce::String s;
        s << "effects: batch=" << (int) core.getBatchCount()
          << " lastUs=" << juce::String (core.getLastBatchUs(), 1)
          << " perWake=" << (int) core.getBatchesPerWake()
          << " skips=" << (int) core.getSourceSkips()
          << " wraps=" << (int) core.getRingWraps()
          << " clears=" << (int) core.getClearCount()
          << " lockFail=" << (int) core.getLockFailures()
          << " workers=" << core.getNumWorkers()
          << " cushion=" << core.getReturnCushionBlocks();

        for (int fx = 0; fx < preparedEffects; ++fx)
        {
            s << "\n  fx " << (fx + 1)
              << " feedPk=" << dB (core.getFeedPeak (fx)) << "dB"
              << " retPk=" << dB (core.getReturnPeak (fx)) << "dB"
              << " under=" << (int) core.getUnderruns (fx)
              << " disc=" << (int) core.getReturnDiscards (fx)
              << " nan=" << (int) core.getNanTrips (fx)
              << " lg=" << (int) core.getLoopGuardTrips (fx) << "/" << (core.isLoopGuardTripped (fx) ? "TRIPPED" : "-")
              << " lat=" << core.getChainLatencySamples (fx)
              << " rev=" << (int) revision[static_cast<size_t> (fx)];
        }
        return s;
    }

    //==========================================================================
    // The cook - pure, testable without a device
    //==========================================================================

    /** One channel's parameters, transcribed from the tree into the engine's
        POD. Field names are the identifiers minus the prefix and every unit
        is the tree's unit (dB stays dB, per cent stays per cent, Hz and ms
        stay), so this is a transcription with three exceptions: the chain
        order string is parsed (a rejected string keeps lastGoodOrder and
        reports orderOk = false), the phaser stage count snaps to what the
        module can build, and a property the node does not carry keeps the
        POD's default, which is the app's default too. */
    static ChannelParams cookChannel (WFSValueTreeState& vts, int fx, ChainOrder& lastGoodOrder, bool& orderOk)
    {
        using namespace WFSParameterIDs;

        ChannelParams p {};
        orderOk = true;

        auto f = [] (const juce::ValueTree& node, const juce::Identifier& key, float& dest)
        {
            if (node.isValid() && node.hasProperty (key))
                dest = static_cast<float> (static_cast<double> (node.getProperty (key)));
        };
        auto u8 = [] (const juce::ValueTree& node, const juce::Identifier& key, std::uint8_t& dest)
        {
            if (node.isValid() && node.hasProperty (key))
                dest = static_cast<std::uint8_t> (juce::jlimit (0, 255, static_cast<int> (node.getProperty (key))));
        };

        // Channel + chain
        u8 (vts.getEffectChannelSection (fx), effectMute, p.mute);

        auto chain = vts.getEffectChainSection (fx);
        u8 (chain, effectChainBypass, p.chainBypass);
        if (chain.isValid() && chain.hasProperty (effectChainOrder))
        {
            const auto text = chain.getProperty (effectChainOrder).toString();
            if (! spatcore::effects::parseChainOrder (text.toRawUTF8(), lastGoodOrder))
                orderOk = false;
        }
        p.order = lastGoodOrder;

        // Distortion
        {
            auto n = vts.getEffectModuleSection (fx, FxDist);
            u8 (n, effectDistBypass, p.dist.bypass);
            u8 (n, effectDistOversample, p.dist.oversample);
            f (n, effectDistDrive, p.dist.driveDb);
            f (n, effectDistShape, p.dist.shape);
            f (n, effectDistBias, p.dist.bias);
            f (n, effectDistPreLoShelfFreq, p.dist.preLoShelfHz);
            f (n, effectDistPreLoShelfGain, p.dist.preLoShelfDb);
            f (n, effectDistPreHiShelfFreq, p.dist.preHiShelfHz);
            f (n, effectDistPreHiShelfGain, p.dist.preHiShelfDb);
            f (n, effectDistPostLoShelfFreq, p.dist.postLoShelfHz);
            f (n, effectDistPostLoShelfGain, p.dist.postLoShelfDb);
            f (n, effectDistPostHiShelfFreq, p.dist.postHiShelfHz);
            f (n, effectDistPostHiShelfGain, p.dist.postHiShelfDb);
            f (n, effectDistOutput, p.dist.outputDb);
            f (n, effectDistMix, p.dist.mix);
        }

        // EQ x 2: the bypass on the module node, the bands on its children
        for (int inst = 0; inst < 2; ++inst)
        {
            auto& eq = p.eq[inst];
            u8 (vts.getEffectEQSection (fx, inst), effectEQBypass, eq.bypass);

            for (int band = 0; band < WFSParameterDefaults::numEffectEQBands && band < 6; ++band)
            {
                auto b = vts.getEffectEQBand (fx, inst, band);
                u8 (b, effectEQshape, eq.shape[band]);
                f (b, effectEQfreq, eq.freqHz[band]);
                f (b, effectEQgain, eq.gainDb[band]);
                f (b, effectEQq, eq.q[band]);
                f (b, effectEQslope, eq.slope[band]);
            }
        }

        // Dynamics x 2
        for (int inst = 0; inst < 2; ++inst)
        {
            auto& d = p.dyn[inst];
            auto n = vts.getEffectDynSection (fx, inst);
            u8 (n, effectDynBypass, d.bypass);
            u8 (n, effectDynDetector, d.detector);
            u8 (n, effectDynAutoMakeup, d.autoMakeup);
            u8 (n, effectDynCompOn, d.compOn);
            u8 (n, effectDynExpOn, d.expOn);
            f (n, effectDynLookahead, d.lookaheadMs);
            f (n, effectDynMakeup, d.makeupDb);
            f (n, effectDynCompThreshold, d.compThresholdDb);
            f (n, effectDynCompRatio, d.compRatio);
            f (n, effectDynCompKnee, d.compKneeDb);
            f (n, effectDynCompAttack, d.compAttackMs);
            f (n, effectDynCompRelease, d.compReleaseMs);
            f (n, effectDynCompDetectorDelay, d.compDetectorDelayMs);
            f (n, effectDynCompScLoCut, d.compScLoCutHz);
            f (n, effectDynCompScHiCut, d.compScHiCutHz);
            f (n, effectDynExpThreshold, d.expThresholdDb);
            f (n, effectDynExpRatio, d.expRatio);
            f (n, effectDynExpAttack, d.expAttackMs);
            f (n, effectDynExpRelease, d.expReleaseMs);
            f (n, effectDynExpRange, d.expRangeDb);
            f (n, effectDynExpHold, d.expHoldMs);
            f (n, effectDynExpScLoCut, d.expScLoCutHz);
            f (n, effectDynExpScHiCut, d.expScHiCutHz);
        }

        // Chorus / flanger
        {
            auto n = vts.getEffectModuleSection (fx, FxMod);
            u8 (n, effectModBypass, p.mod.bypass);
            u8 (n, effectModMode, p.mod.mode);
            u8 (n, effectModVoices, p.mod.voices);
            u8 (n, effectModShape, p.mod.shape);
            u8 (n, effectModThroughZero, p.mod.throughZero);
            f (n, effectModRate, p.mod.rateHz);
            f (n, effectModDepth, p.mod.depth);
            f (n, effectModDelay, p.mod.delayMs);
            f (n, effectModFeedback, p.mod.feedback);
            f (n, effectModPhase, p.mod.phaseDeg);
            f (n, effectModLoCut, p.mod.loCutHz);
            f (n, effectModMix, p.mod.mix);
        }

        // Phaser: the tree bounds the stage count as a range (4..12), the
        // module builds 4, 6, 8 or 12 - snap to the nearest, ties down
        {
            auto n = vts.getEffectModuleSection (fx, FxPhaser);
            u8 (n, effectPhaserBypass, p.phaser.bypass);
            u8 (n, effectPhaserShape, p.phaser.shape);
            std::uint8_t stages = p.phaser.stages;
            u8 (n, effectPhaserStages, stages);
            p.phaser.stages = snapPhaserStages (stages);
            f (n, effectPhaserCentre, p.phaser.centreHz);
            f (n, effectPhaserSpread, p.phaser.spreadOct);
            f (n, effectPhaserRate, p.phaser.rateHz);
            f (n, effectPhaserDepth, p.phaser.depthOct);
            f (n, effectPhaserFeedback, p.phaser.feedback);
            f (n, effectPhaserMix, p.phaser.mix);
        }

        // Tremolo
        {
            auto n = vts.getEffectModuleSection (fx, FxTrem);
            u8 (n, effectTremBypass, p.trem.bypass);
            f (n, effectTremRate, p.trem.rateHz);
            f (n, effectTremDepth, p.trem.depthDb);
            f (n, effectTremShape, p.trem.shape);
            f (n, effectTremMix, p.trem.mix);
        }

        // Reverb
        {
            auto n = vts.getEffectModuleSection (fx, FxReverb);
            u8 (n, effectReverbBypass, p.reverb.bypass);
            u8 (n, effectReverbModel, p.reverb.model);
            u8 (n, effectReverbType, p.reverb.type);
            f (n, effectReverbPredelay, p.reverb.predelayMs);
            f (n, effectReverbRT60, p.reverb.rt60);
            f (n, effectReverbRT60LowMult, p.reverb.rt60LowMult);
            f (n, effectReverbRT60HighMult, p.reverb.rt60HighMult);
            f (n, effectReverbCrossoverLow, p.reverb.crossoverLow);
            f (n, effectReverbCrossoverHigh, p.reverb.crossoverHigh);
            f (n, effectReverbDiffusion, p.reverb.diffusion);
            f (n, effectReverbSize, p.reverb.size);
            f (n, effectReverbTone, p.reverb.toneHz);
            f (n, effectReverbMix, p.reverb.mix);
        }

        // Multitap delay: the module node plus its eight <Tap> children
        {
            auto n = vts.getEffectModuleSection (fx, FxDelay);
            u8 (n, effectDelayBypass, p.delay.bypass);
            u8 (n, effectDelayTaps, p.delay.taps);
            u8 (n, effectDelayTapMode, p.delay.tapMode);
            u8 (n, effectDelayPattern, p.delay.pattern);
            u8 (n, effectDelayFeedbackTap, p.delay.feedbackTap);
            f (n, effectDelayTime, p.delay.timeMs);
            f (n, effectDelayFeedback, p.delay.feedback);
            f (n, effectDelayInLoCut, p.delay.inLoCutHz);
            f (n, effectDelayFbLoShelfFreq, p.delay.fbLoShelfHz);
            f (n, effectDelayFbLoShelfGain, p.delay.fbLoShelfDb);
            f (n, effectDelayFbHiShelfFreq, p.delay.fbHiShelfHz);
            f (n, effectDelayFbHiShelfGain, p.delay.fbHiShelfDb);
            f (n, effectDelayModRate, p.delay.modRateHz);
            f (n, effectDelayModDepth, p.delay.modDepthPct);
            f (n, effectDelayDiffusion, p.delay.diffusion);
            f (n, effectDelayGlide, p.delay.glideMs);
            f (n, effectDelayMix, p.delay.mix);

            for (int tap = 0; tap < WFSParameterDefaults::numEffectDelayTaps && tap < 8; ++tap)
            {
                auto t = vts.getEffectDelayTap (fx, tap);
                f (t, effectDelayTapTime, p.delay.tapTimeMs[tap]);
                f (t, effectDelayTapLevel, p.delay.tapLevelDb[tap]);
            }
        }

        // Bitcrusher
        {
            auto n = vts.getEffectModuleSection (fx, FxCrush);
            u8 (n, effectCrushBypass, p.crush.bypass);
            u8 (n, effectCrushFilter, p.crush.filter);
            f (n, effectCrushBits, p.crush.bits);
            f (n, effectCrushRate, p.crush.rateHz);
            f (n, effectCrushDither, p.crush.ditherDb);
            f (n, effectCrushMix, p.crush.mix);
        }

        return p;
    }

    /** The phaser builds 4, 6, 8 or 12 stages; the tree bounds the value as a
        range. Nearest, ties down: 5 -> 4, 7 -> 6, 10 -> 8, 11 -> 12. */
    static std::uint8_t snapPhaserStages (int stages) noexcept
    {
        static constexpr int allowed[] = { 4, 6, 8, 12 };
        int best = allowed[0];
        for (int candidate : allowed)
            if (std::abs (candidate - stages) < std::abs (best - stages))
                best = candidate;
        return static_cast<std::uint8_t> (best);
    }

    /** The publish count of one channel (test hook). */
    std::uint32_t getRevision (int fx) const noexcept
    {
        return fx >= 0 && fx < kMaxEffects ? revision[static_cast<size_t> (fx)] : 0;
    }

    /** Take (and clear) the dirty mask without publishing (test hook). */
    std::uint32_t takeDirtyMaskForTest() noexcept { return dirtyMask.exchange (0); }

private:
    //==========================================================================
    // ValueTree::Listener: any change under an <Effect> dirties that channel.
    // The walk up is a few parent hops per change anywhere in the tree; a
    // change to a <Sends> row cooks a no-op (the sends live in the
    // calculation engine's matrices), which is cheap enough not to special-case.
    //==========================================================================
    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier&) override { markIfUnderEffect (tree); }
    void valueTreeChildAdded (juce::ValueTree& parent, juce::ValueTree&) override { markIfUnderEffect (parent); }
    void valueTreeChildRemoved (juce::ValueTree& parent, juce::ValueTree&, int) override { markIfUnderEffect (parent); }
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged (juce::ValueTree&) override {}

    void markIfUnderEffect (const juce::ValueTree& node)
    {
        // Up to the channel, then the same count-by-type walk every effect
        // accessor uses - never id - 1, which a merged file can contradict
        juce::ValueTree effect = node;
        while (effect.isValid() && ! effect.hasType (WFSParameterIDs::Effect))
            effect = effect.getParent();

        if (! effect.isValid())
            return;

        auto parent = effect.getParent();
        if (! parent.isValid())
            return;

        int index = 0;
        for (int i = 0; i < parent.getNumChildren(); ++i)
        {
            auto child = parent.getChild (i);
            if (child == effect)
            {
                if (index < kMaxEffects)
                    dirtyMask.fetch_or (1u << index);
                return;
            }
            if (child.hasType (WFSParameterIDs::Effect))
                ++index;
        }
    }

    WFSValueTreeState& valueTreeState;
    Engine engine;

    int preparedEffects = 0;
    int preparedSources = 0;
    int firstSlot = -1;

    std::array<std::uint32_t, kMaxEffects> revision {};
    std::array<ChainOrder, kMaxEffects> lastGoodOrder {};
    std::array<juce::String, kMaxEffects> lastLoggedBadOrder;
    std::atomic<std::uint32_t> dirtyMask { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectsHost)
};
