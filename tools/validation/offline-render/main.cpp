//==============================================================================
// offline-render — the bit-exact gate for the spatcore extraction (Phase 0).
// Design: docs/architecture/offline-render-harness.md
//
// Renders scripted deterministic scenarios through the CPU WFS renderers
// (gather = InputBufferProcessor, scatter = OutputBufferProcessor) and the
// three reverb algorithms (SDN / FDN / IR), entirely headless, then prints
// the SHA-256 of the raw float32 PCM (little-endian, channel-major byte dump)
// of all output channels.
//
//   offline-render --path <cpu-gather|cpu-scatter|reverb-sdn|reverb-fdn|reverb-ir
//                          |effects|gpu-gather|gpu-scatter|gpu-reverb-sdn
//                          |gpu-reverb-fdn|gpu-reverb-ir|cpu|gpu|all>
//                  --scenario <static|moving|fr-toggle|stereo|all
//                              |dist|eq|dyn|mod|phaser|trem|reverb|delay|crush|chain>
//                  [--blocks N] [--block 512] [--sr 48000] [--in 8] [--out 16]
//                  [--device cuda:0] [--plugin-dir <dir with wfs_cuda.dll>]
//                  [--wav out.wav] [--raw out.f32]
//                  [--check baselines/<machine>.json] [--update]
//                  [--bench] [--warmup 16] [--bench-json <file>]
//
// --check compares each rendered hash against the committed JSON baseline and
// exits 1 on any mismatch (same contract as tools/validation/kernel_hashes.py);
// --check with --update rewrites the baseline entries for the combos just run.
//
// Both apply at the DEFAULT render shape only: the shape is stamped into the
// baseline file under the reserved "#shape" key, and any other --blocks /
// --block / --sr / --in / --out is refused with exit 2. A wrong-shape --check
// would only MISMATCH, but a wrong-shape --update would quietly record a hash
// of a truncated run (30 blocks of an effects scenario never reach the end of
// the bypass window, let alone a variant switch) as the golden and exit 0.
//
// If an effects render trips its NaN trap, the hash is the trap's output and
// not the module's, so the run exits 8 and neither checks nor records.
//
// --bench (GPU host-path optimization M0) reports per path x scenario: blocks,
// wall ms, xRealtime, per-block budget ms, and — for GPU paths — the
// distribution of backend->getLastLaunchMs() (min/median/p99/max/mean, via
// nth_element). The first --warmup blocks (default 16) are excluded from wall
// and launch stats. Hashes still print; only the default baseline shape is
// baselined — bench shapes (e.g. 96k/128/64x128) are NOT meant for --check.
//
// The harness compiles the app's DSP headers in place and drives them exactly
// as the app does (drain-pull below the async algorithm wrappers) — no
// production-code changes.
//
// The effects path (--path effects) is the same gate for spatcore/effects: one
// scenario per module driven through a ModuleSlot, plus one whole-chain
// scenario that reorders the chain and toggles chain bypass/mute mid-render.
// Modules are mono and synchronous, so it renders --in independent chains (one
// per input stream, each with its own ChainConfig::noiseKey) and hashes them
// exactly like every other path. Scenario families do not cross: --scenario
// all means the four WFS timelines on a render path and the ten effects
// timelines on --path effects.
//
// GPU paths (milestone 2, WFS_GPU_NATIVE builds): drive the vendor backends
// SYNCHRONOUSLY — makeWfsBackend/makeObBackend(deviceId) ->
// prepare(..., pipelineLatencyMs = 0, ...) -> setMatrixPointers ->
// processBlock — bypassing NativeGpu*Algorithm / GpuAsyncPipelineT entirely.
// Like the app (WFS_GPU_PLUGINS), no GPU runtime is linked: GpuBackendFactory
// dlopens wfs_cuda.dll / wfs_hip.dll at runtime (--plugin-dir, or an
// auto-probed Builds/VisualStudio2022 output dir, is added to the DLL search
// path). GPU hashes are a SELF-CONSISTENCY gate only (same device + driver);
// keep them in a separate baseline file (baselines/<machine>-gpu.json) checked
// in a separate invocation (--path gpu --check ...) so the CPU baseline stays
// portable. With --path all, GPU paths are SKIPPED with a note when no
// GPU/plugin is present; explicitly requested gpu paths fail with exit 6.
//==============================================================================

#include <JuceHeader.h>

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../../../spatcore/wfs/InputBufferProcessor.h"     // CPU gather (per-input worker threads)
#include "../../../spatcore/wfs/OutputBufferProcessor.h"    // CPU scatter (per-output worker threads)
#include "../../../spatcore/rt/SharedInputRingBuffer.h"
#include "../../../spatcore/rt/AudioParallelFor.h"
#include "../../../spatcore/reverb/ReverbAlgorithm.h"
#include "../../../spatcore/reverb/ReverbSDNAlgorithm.h"
#include "../../../spatcore/reverb/ReverbFDNAlgorithm.h"
#include "../../../spatcore/reverb/ReverbIRAlgorithm.h"
#include "../../../spatcore/effects/EffectChain.h"          // EffectChain, ModuleSlot, createModule

#if WFS_GPU_NATIVE
 #include "../../../spatcore/gpu/GpuDeviceManager.h"   // device enumeration ("cuda:0", ...)
 #include "../../../spatcore/gpu/WfsGpuBackend.h"      // makeWfsBackend (gather)
 #include "../../../spatcore/gpu/ObGpuBackend.h"       // makeObBackend  (scatter)
 #include "../../../spatcore/gpu/SdnGpuBackend.h"      // makeSdnBackend (reverb)
 #include "../../../spatcore/gpu/FdnGpuBackend.h"      // makeFdnBackend (reverb)
 #include "../../../spatcore/gpu/IrGpuBackend.h"       // makeIrBackend  (reverb)
#endif

#include "scenarios.h"
#include "sha256.h"

//==============================================================================
namespace
{

/** NaN traps tripped by every effects render in this invocation. Any non-zero
    value invalidates the run - see renderEffects() and exit code 8. */
std::uint32_t gNanTripTotal = 0;

struct Config
{
    double sr = 48000.0;
    int block = 512;
    int blocks = 200;
    int numIn = 8;
    int numOut = 16;
    int reverbWorkers = 0;   // AudioParallelFor width for the CPU reverb paths
};

/** The render shape a hash was produced at, as it is stored in the baseline
    file under kShapeKey.

    A hash means nothing without it: a 30-block run of an effects scenario
    stops at tick ~31, before the bypass window closes and long before either
    variant switch, so it hashes a fraction of the script the golden is
    supposed to gate. --check at the wrong shape is at least loud (it
    MISMATCHes), but --update at the wrong shape would quietly record that
    fraction as the golden and exit 0, which is why both are refused. */
const char* const kShapeKey = "#shape";

std::string shapeString (const Config& cfg)
{
    char buf[128];
    std::snprintf (buf, sizeof (buf),
                   "sr=%.0f block=%d blocks=%d in=%d out=%d",
                   cfg.sr, cfg.block, cfg.blocks, cfg.numIn, cfg.numOut);
    return buf;
}

enum class Path
{
    CpuGather,
    CpuScatter,
    ReverbSdn,
    ReverbFdn,
    ReverbIr,
    Effects,
    GpuGather,
    GpuScatter,
    GpuReverbSdn,
    GpuReverbFdn,
    GpuReverbIr,
};

const char* pathName (Path p)
{
    switch (p)
    {
        case Path::CpuGather:    return "cpu-gather";
        case Path::CpuScatter:   return "cpu-scatter";
        case Path::ReverbSdn:    return "reverb-sdn";
        case Path::ReverbFdn:    return "reverb-fdn";
        case Path::ReverbIr:     return "reverb-ir";
        case Path::Effects:      return "effects";
        case Path::GpuGather:    return "gpu-gather";
        case Path::GpuScatter:   return "gpu-scatter";
        case Path::GpuReverbSdn: return "gpu-reverb-sdn";
        case Path::GpuReverbFdn: return "gpu-reverb-fdn";
        case Path::GpuReverbIr:  return "gpu-reverb-ir";
    }
    return "?";
}

bool pathFromName (const std::string& s, Path& out)
{
    if (s == "cpu-gather")     { out = Path::CpuGather;    return true; }
    if (s == "cpu-scatter")    { out = Path::CpuScatter;   return true; }
    if (s == "reverb-sdn")     { out = Path::ReverbSdn;    return true; }
    if (s == "reverb-fdn")     { out = Path::ReverbFdn;    return true; }
    if (s == "reverb-ir")      { out = Path::ReverbIr;     return true; }
    if (s == "effects")        { out = Path::Effects;     return true; }
    if (s == "gpu-gather")     { out = Path::GpuGather;    return true; }
    if (s == "gpu-scatter")    { out = Path::GpuScatter;   return true; }
    if (s == "gpu-reverb-sdn") { out = Path::GpuReverbSdn; return true; }
    if (s == "gpu-reverb-fdn") { out = Path::GpuReverbFdn; return true; }
    if (s == "gpu-reverb-ir")  { out = Path::GpuReverbIr;  return true; }
    return false;
}

bool isGpuPath (Path p)
{
    return p == Path::GpuGather || p == Path::GpuScatter
        || p == Path::GpuReverbSdn || p == Path::GpuReverbFdn || p == Path::GpuReverbIr;
}

const std::vector<Path>& cpuPaths()
{
    static const std::vector<Path> v {
        Path::CpuGather, Path::CpuScatter,
        Path::ReverbSdn, Path::ReverbFdn, Path::ReverbIr };
    return v;
}

/** --path all = every path that does not need a GPU, then the GPU ones.
    cpuPaths() is deliberately NOT widened: --path cpu names the five WFS and
    reverb render paths, and widening it would change what the documented CPU
    baseline invocation renders. */
const std::vector<Path>& cpuOnlyPaths()
{
    static const std::vector<Path> v {
        Path::CpuGather, Path::CpuScatter,
        Path::ReverbSdn, Path::ReverbFdn, Path::ReverbIr,
        Path::Effects };
    return v;
}

const std::vector<Path>& gpuPaths()
{
    static const std::vector<Path> v {
        Path::GpuGather, Path::GpuScatter,
        Path::GpuReverbSdn, Path::GpuReverbFdn, Path::GpuReverbIr };
    return v;
}

using ChannelData = std::vector<std::vector<float>>;   // [channel][sample]

//==============================================================================
// --bench: per-combo wall clock + per-block launchMs distribution (M0 of the
// GPU host-path optimization). The render loops call blockBegin/blockEnd once
// per block; blocks below the warmup threshold are excluded from every stat.
// GPU paths pass backend->getLastLaunchMs() to blockEnd; CPU paths pass a
// negative sentinel (wall/xRealtime only — no launch distribution).
//==============================================================================
struct Bench
{
    bool enabled = false;
    int warmup = 16;                    // blocks excluded from all stats

    struct LaunchStats
    {
        bool valid = false;
        double minMs = 0.0, medianMs = 0.0, p99Ms = 0.0, maxMs = 0.0, meanMs = 0.0;
    };

    struct Result
    {
        int blocks = 0;                 // measured (post-warmup) blocks
        double wallMs = 0.0;
        double xRealtime = 0.0;
        double budgetMs = 0.0;
        LaunchStats launch;
    };

    void beginCombo (const Config& cfg)
    {
        if (! enabled)
            return;
        effWarmup = std::max (0, std::min (warmup, cfg.blocks - 1));
        if (effWarmup != warmup)
            std::fprintf (stderr, "note: bench warmup clamped to %d (%d blocks total)\n",
                          effWarmup, cfg.blocks);
        launchMs.clear();
        launchMs.reserve (static_cast<size_t> (cfg.blocks - effWarmup));
        wallStart = 0.0;
        wallMs = 0.0;
        measured = 0;
    }

    /** Top of the per-block loop body. Starts the wall clock when the first
        measured block begins. */
    void blockBegin (int b)
    {
        if (enabled && b == effWarmup)
            wallStart = juce::Time::getMillisecondCounterHiRes();
    }

    /** Bottom of the per-block loop body. launchMsValue < 0 => CPU path. */
    void blockEnd (int b, double launchMsValue)
    {
        if (! enabled || b < effWarmup)
            return;
        ++measured;
        wallMs = juce::Time::getMillisecondCounterHiRes() - wallStart;
        if (launchMsValue >= 0.0)
            launchMs.push_back (launchMsValue);
    }

    /** Print + record the just-rendered combo (call after renderOne). */
    void report (const std::string& key, const Config& cfg)
    {
        if (! enabled)
            return;

        Result r;
        r.blocks = measured;
        r.wallMs = wallMs;
        r.budgetMs = 1000.0 * cfg.block / cfg.sr;
        const double audioSeconds = static_cast<double> (measured) * cfg.block / cfg.sr;
        r.xRealtime = wallMs > 0.0 ? audioSeconds / (wallMs / 1000.0) : 0.0;

        if (! launchMs.empty())
        {
            std::vector<double> v = launchMs;   // nth_element permutes
            const size_t n = v.size();
            auto nth = [&v] (size_t idx) { std::nth_element (v.begin(), v.begin() + (long) idx, v.end()); return v[idx]; };

            r.launch.valid = true;
            r.launch.medianMs = nth (n / 2);
            r.launch.p99Ms = nth (std::min (n - 1, static_cast<size_t> (std::llround (0.99 * static_cast<double> (n - 1)))));
            const auto mm = std::minmax_element (v.begin(), v.end());
            r.launch.minMs = *mm.first;
            r.launch.maxMs = *mm.second;
            double sum = 0.0;
            for (double d : v) sum += d;
            r.launch.meanMs = sum / static_cast<double> (n);
        }

        if (r.launch.valid)
            std::printf ("bench %s blocks=%d wallMs=%.2f xRealtime=%.2f budgetMs=%.4f "
                         "launchMs[min=%.4f med=%.4f p99=%.4f max=%.4f mean=%.4f]\n",
                         key.c_str(), r.blocks, r.wallMs, r.xRealtime, r.budgetMs,
                         r.launch.minMs, r.launch.medianMs, r.launch.p99Ms,
                         r.launch.maxMs, r.launch.meanMs);
        else
            std::printf ("bench %s blocks=%d wallMs=%.2f xRealtime=%.2f budgetMs=%.4f\n",
                         key.c_str(), r.blocks, r.wallMs, r.xRealtime, r.budgetMs);
        std::fflush (stdout);

        results[key] = r;
    }

    /** Optional machine-readable dump (--bench-json). */
    bool writeJson (const juce::File& f, const Config& cfg) const
    {
        juce::String json;
        json << "{\n"
             << "  \"sr\": " << cfg.sr << ",\n"
             << "  \"block\": " << cfg.block << ",\n"
             << "  \"blocks\": " << cfg.blocks << ",\n"
             << "  \"in\": " << cfg.numIn << ",\n"
             << "  \"out\": " << cfg.numOut << ",\n"
             << "  \"warmup\": " << warmup << ",\n"
             << "  \"results\": {\n";
        size_t i = 0;
        for (const auto& e : results)
        {
            const Result& r = e.second;
            json << "    \"" << juce::String (e.first) << "\": { "
                 << "\"blocks\": " << r.blocks
                 << ", \"wallMs\": " << juce::String (r.wallMs, 3)
                 << ", \"xRealtime\": " << juce::String (r.xRealtime, 3)
                 << ", \"budgetMs\": " << juce::String (r.budgetMs, 5);
            if (r.launch.valid)
                json << ", \"launchMs\": { \"min\": " << juce::String (r.launch.minMs, 5)
                     << ", \"median\": " << juce::String (r.launch.medianMs, 5)
                     << ", \"p99\": " << juce::String (r.launch.p99Ms, 5)
                     << ", \"max\": " << juce::String (r.launch.maxMs, 5)
                     << ", \"mean\": " << juce::String (r.launch.meanMs, 5) << " }";
            json << " }";
            if (++i < results.size()) json << ",";
            json << "\n";
        }
        json << "  }\n}\n";

        f.getParentDirectory().createDirectory();
        return f.replaceWithText (json);
    }

private:
    int effWarmup = 0;
    int measured = 0;
    double wallStart = 0.0;
    double wallMs = 0.0;
    std::vector<double> launchMs;
    std::map<std::string, Result> results;
};

Bench gBench;

//==============================================================================
// Drain-pull with a bounded spin: a hung worker fails the gate loudly instead
// of deadlocking CI (design doc "Open implementation details").
//==============================================================================
template <typename PullFn>
void drainPull (PullFn&& pull, float* dest, int numSamples, const char* what)
{
    int got = 0;
    const double start = juce::Time::getMillisecondCounterHiRes();

    while (got < numSamples)
    {
        const int r = pull (dest + got, numSamples - got);
        got += r;
        if (got >= numSamples)
            break;

        if (r == 0)
        {
            if (juce::Time::getMillisecondCounterHiRes() - start > 5000.0)
            {
                std::fprintf (stderr,
                    "FATAL: drain-pull timeout (%s): got %d/%d samples after 5 s "
                    "— worker thread hung or stalled\n", what, got, numSamples);
                std::exit (3);
            }
            juce::Thread::yield();
        }
    }
}

/** 50 Hz tick index for a stream position (pure integer function). */
int tickForSample (int64_t sampleIndex, int srInt)
{
    return static_cast<int> ((sampleIndex * 50) / srInt);
}

//==============================================================================
// CPU gather: one InputBufferProcessor per input. Drive pattern mirrors
// InputBufferAlgorithm::prepare/processBlock (InputBufferAlgorithm.h:49-79 and
// :137-158): push all inputs, then drain-pull each (in, out) pair in the app's
// exact in-outer / out-inner loop order and sum sequentially, which fixes the
// float summation order.
//==============================================================================
ChannelData renderCpuGather (scenario::Id id, const Config& cfg)
{
    const int srInt = static_cast<int> (cfg.sr);
    scenario::WfsMatrices m;
    m.allocate (cfg.numIn, cfg.numOut);
    scenario::applyWfsTick (id, 0, cfg.numIn, cfg.numOut, m);

    std::vector<std::unique_ptr<InputBufferProcessor>> procs;
    for (int i = 0; i < cfg.numIn; ++i)
    {
        auto p = std::make_unique<InputBufferProcessor> (i, cfg.numOut,
                     m.delayMs.data(), m.levels.data(), m.hfDb.data(),
                     m.frDelayMs.data(), m.frLevels.data(), m.frHfDb.data());
        p->prepare (cfg.sr, cfg.block);
        procs.push_back (std::move (p));
    }

    // Constant per-run FR processor settings (set from the "timer thread" side
    // before the workers start; frLevels toggling does the mid-run switching).
    const auto fr = scenario::frSettings (id);
    for (auto& p : procs)
    {
        p->setFRFilterParams (fr.lowCutActive, fr.lowCutFreq,
                              fr.highShelfActive, fr.highShelfFreq,
                              fr.highShelfGain, fr.highShelfSlope);
        p->setFRDiffusion (fr.diffusionPercent);
    }

    // Start threads AFTER all processors are created and prepared
    // (InputBufferAlgorithm.h:72-78).
    for (auto& p : procs)
    {
        p->setProcessingEnabled (true);
        if (! p->startRealtimeThread (juce::Thread::RealtimeOptions{}
                                          .withApproximateAudioProcessingTime (cfg.block, cfg.sr)))
        {
            std::fprintf (stderr, "warning: startRealtimeThread failed, using normal priority\n");
            p->startThread();
        }
    }

    const int64_t total = static_cast<int64_t> (cfg.blocks) * cfg.block;
    ChannelData out (static_cast<size_t> (cfg.numOut),
                     std::vector<float> (static_cast<size_t> (total), 0.0f));

    std::vector<float> inChan (static_cast<size_t> (cfg.block));
    std::vector<float> tmp (static_cast<size_t> (cfg.block));
    int lastTick = 0;   // tick 0 already applied

    for (int b = 0; b < cfg.blocks; ++b)
    {
        gBench.blockBegin (b);
        const int64_t startSample = static_cast<int64_t> (b) * cfg.block;

        // Matrix timeline: re-write the arrays at tick boundaries, between
        // blocks only (all pushed samples are fully drained => workers idle).
        const int tick = tickForSample (startSample, srInt);
        if (tick != lastTick)
        {
            scenario::applyWfsTick (id, tick, cfg.numIn, cfg.numOut, m);
            lastTick = tick;
        }

        for (int in = 0; in < cfg.numIn; ++in)
        {
            for (int s = 0; s < cfg.block; ++s)
                inChan[static_cast<size_t> (s)] =
                    scenario::inputSample (id, in, startSample + s, cfg.sr);
            procs[static_cast<size_t> (in)]->pushInput (inChan.data(), cfg.block);
        }

        // App loop order: in outer, out inner (InputBufferAlgorithm.h:137-158)
        for (int in = 0; in < cfg.numIn; ++in)
        {
            for (int outCh = 0; outCh < cfg.numOut; ++outCh)
            {
                auto* proc = procs[static_cast<size_t> (in)].get();
                drainPull ([proc, outCh] (float* d, int n)
                           { return proc->pullOutput (outCh, d, n); },
                           tmp.data(), cfg.block, "cpu-gather");

                float* dst = out[static_cast<size_t> (outCh)].data() + startSample;
                for (int s = 0; s < cfg.block; ++s)
                    dst[s] += tmp[static_cast<size_t> (s)];
            }
        }
        gBench.blockEnd (b, -1.0);
    }

    for (auto& p : procs)
        p->stopThread (1000);

    return out;
}

//==============================================================================
// CPU scatter: mirrors OutputBufferAlgorithm::prepare (OutputBufferAlgorithm.h:
// 214-245: shared input rings sized blockSize*4, processors prepared and wired
// before their threads start) and its processBlock write/notify/pull sequence
// (:329-353). The metering/analysis side-threads are not started — they only
// feed level detectors, never the audio path (the metering ring just drops
// samples once full).
//==============================================================================
ChannelData renderCpuScatter (scenario::Id id, const Config& cfg)
{
    const int srInt = static_cast<int> (cfg.sr);
    scenario::WfsMatrices m;
    m.allocate (cfg.numIn, cfg.numOut);
    scenario::applyWfsTick (id, 0, cfg.numIn, cfg.numOut, m);

    // Shared input ring buffers (one per input, read by all output threads) —
    // OutputBufferAlgorithm.h:214-221.
    std::vector<std::unique_ptr<SharedInputRingBuffer>> rings;
    for (int i = 0; i < cfg.numIn; ++i)
    {
        auto buf = std::make_unique<SharedInputRingBuffer>();
        buf->setSize (cfg.block * 4);
        rings.push_back (std::move (buf));
    }

    std::vector<std::unique_ptr<OutputBufferProcessor>> procs;
    for (int o = 0; o < cfg.numOut; ++o)
    {
        auto p = std::make_unique<OutputBufferProcessor> (o, cfg.numIn, cfg.numOut,
                     m.delayMs.data(), m.levels.data(), m.hfDb.data(),
                     m.frDelayMs.data(), m.frLevels.data(), m.frHfDb.data());
        p->prepare (cfg.sr, cfg.block);
        p->setSharedInputBuffers (rings);
        procs.push_back (std::move (p));
    }

    const auto fr = scenario::frSettings (id);
    for (auto& p : procs)
        for (int in = 0; in < cfg.numIn; ++in)
        {
            p->setFRFilterParams (in, fr.lowCutActive, fr.lowCutFreq,
                                  fr.highShelfActive, fr.highShelfFreq,
                                  fr.highShelfGain, fr.highShelfSlope);
            p->setFRDiffusion (in, fr.diffusionPercent);
        }

    // Start threads AFTER all processors are created and prepared
    // (OutputBufferAlgorithm.h:238-245).
    for (auto& p : procs)
    {
        p->setProcessingEnabled (true);
        if (! p->startRealtimeThread (juce::Thread::RealtimeOptions{}
                                          .withApproximateAudioProcessingTime (cfg.block, cfg.sr)))
        {
            std::fprintf (stderr, "warning: startRealtimeThread failed, using normal priority\n");
            p->startThread();
        }
    }

    const int64_t total = static_cast<int64_t> (cfg.blocks) * cfg.block;
    ChannelData out (static_cast<size_t> (cfg.numOut),
                     std::vector<float> (static_cast<size_t> (total), 0.0f));

    std::vector<float> inChan (static_cast<size_t> (cfg.block));
    int lastTick = 0;

    for (int b = 0; b < cfg.blocks; ++b)
    {
        gBench.blockBegin (b);
        const int64_t startSample = static_cast<int64_t> (b) * cfg.block;

        const int tick = tickForSample (startSample, srInt);
        if (tick != lastTick)
        {
            scenario::applyWfsTick (id, tick, cfg.numIn, cfg.numOut, m);
            lastTick = tick;
        }

        // Write input once to the shared rings, then notify all output threads
        // (OutputBufferAlgorithm.h:329-339).
        for (int in = 0; in < cfg.numIn; ++in)
        {
            for (int s = 0; s < cfg.block; ++s)
                inChan[static_cast<size_t> (s)] =
                    scenario::inputSample (id, in, startSample + s, cfg.sr);
            rings[static_cast<size_t> (in)]->write (inChan.data(), cfg.block);
        }
        for (auto& p : procs)
            p->notifyInputAvailable (cfg.block);

        // Drain-pull each output fully (no cross-channel summation in scatter).
        for (int outCh = 0; outCh < cfg.numOut; ++outCh)
        {
            auto* proc = procs[static_cast<size_t> (outCh)].get();
            drainPull ([proc] (float* d, int n) { return proc->pullOutput (d, n); },
                       out[static_cast<size_t> (outCh)].data() + startSample,
                       cfg.block, "cpu-scatter");
        }
        gBench.blockEnd (b, -1.0);
    }

    for (auto& p : procs)
        p->stopThread (1000);

    return out;
}

//==============================================================================
// Reverb (SDN / FDN / IR): instantiate the algorithm directly and call
// processBlock synchronously — bypasses ReverbEngine's thread/rings/cushion.
// AudioParallelFor is prepared with 0 workers (sequential fallback).
//==============================================================================

/** Waits until every node's convolver has installed the loaded IR.
    juce::dsp::Convolution builds engines on a background thread and installs
    them inside process(), so the install block index is timing-dependent.
    We probe (impulse block + silent blocks) until every node shows a
    convolution tail — the pre-load unit-impulse engine has none — then reset()
    the algorithm, which clears all convolver state and kills the install
    crossfade (Convolution::reset -> mixer.reset + engine reset). Everything
    after the reset is deterministic. */
void waitForIrInstall (ReverbAlgorithm& algo, int numNodes, int block)
{
    juce::AudioBuffer<float> in (numNodes, block), out (numNodes, block);
    const double start = juce::Time::getMillisecondCounterHiRes();

    for (;;)
    {
        // One impulse block...
        in.clear();
        for (int n = 0; n < numNodes; ++n)
            in.setSample (n, 0, 1.0f);
        out.clear();
        algo.processBlock (in, out, block);

        // ...then silent blocks: any energy here is IR tail (engine installed).
        std::vector<float> tailPeak (static_cast<size_t> (numNodes), 0.0f);
        in.clear();
        for (int k = 0; k < 4; ++k)
        {
            out.clear();
            algo.processBlock (in, out, block);
            for (int n = 0; n < numNodes; ++n)
                tailPeak[static_cast<size_t> (n)] = juce::jmax (
                    tailPeak[static_cast<size_t> (n)],
                    out.getMagnitude (n, 0, block));
        }

        bool all = true;
        for (int n = 0; n < numNodes; ++n)
            all = all && (tailPeak[static_cast<size_t> (n)] > 1.0e-6f);
        if (all)
            break;

        if (juce::Time::getMillisecondCounterHiRes() - start > 10000.0)
        {
            std::fprintf (stderr,
                "FATAL: IR convolver install timeout (10 s) — background loader stalled\n");
            std::exit (4);
        }
        juce::Thread::sleep (5);
    }

    algo.reset();   // deterministic zero state, IR engines installed
}

ChannelData renderReverb (Path path, scenario::Id id, const Config& cfg)
{
    const int srInt = static_cast<int> (cfg.sr);
    const int nodes = cfg.numIn;

    // Default 0 workers -> sequential fallback (worker-count-invariant by
    // design, so the baseline hashes are the sequential ones). --reverb-workers
    // drives the real fork-join pool, both to MEASURE how the algorithms scale
    // with worker count (ReverbEngine.h clamps its own pool to 7, which is the
    // binding constraint at high node counts) and to VERIFY that invariance
    // claim: the hash must not move as the count changes.
    AudioParallelFor pool;
    {
        const double blockMs = cfg.sr > 0.0 ? 1000.0 * cfg.block / cfg.sr : 0.0;
        pool.prepare (cfg.reverbWorkers, blockMs, blockMs);
    }

    std::unique_ptr<ReverbAlgorithm> algo;
    switch (path)
    {
        case Path::ReverbSdn: algo = std::make_unique<SDNAlgorithm>(); break;
        case Path::ReverbFdn: algo = std::make_unique<FDNAlgorithm>(); break;
        case Path::ReverbIr:  algo = std::make_unique<IRAlgorithm>();  break;
        default: return {};
    }

    algo->setParallelFor (&pool);
    algo->prepare (cfg.sr, cfg.block, nodes);
    algo->updateGeometry (scenario::nodeBox (nodes));   // SDN uses it; FDN/IR ignore
    algo->setParameters (scenario::reverbParams (id, 0));

    if (path == Path::ReverbIr)
    {
        auto ir = scenario::deterministicIr (cfg.sr);
        juce::AudioBuffer<float> irBuf (1, static_cast<int> (ir.size()));
        irBuf.copyFrom (0, 0, ir.data(), static_cast<int> (ir.size()));

        auto& irAlgo = static_cast<IRAlgorithm&> (*algo);
        irAlgo.loadIRFromBuffer (juce::File(), std::move (irBuf), cfg.sr);
        waitForIrInstall (*algo, nodes, cfg.block);
    }
    else
    {
        algo->reset();
    }

    const int64_t total = static_cast<int64_t> (cfg.blocks) * cfg.block;
    ChannelData out (static_cast<size_t> (nodes),
                     std::vector<float> (static_cast<size_t> (total), 0.0f));

    juce::AudioBuffer<float> nodeIn (nodes, cfg.block), nodeOut (nodes, cfg.block);
    int lastTick = 0;

    for (int b = 0; b < cfg.blocks; ++b)
    {
        gBench.blockBegin (b);
        const int64_t startSample = static_cast<int64_t> (b) * cfg.block;

        const int tick = tickForSample (startSample, srInt);
        if (tick != lastTick)
        {
            algo->setParameters (scenario::reverbParams (id, tick));
            lastTick = tick;
        }

        for (int n = 0; n < nodes; ++n)
        {
            float* dst = nodeIn.getWritePointer (n);
            for (int s = 0; s < cfg.block; ++s)
                dst[s] = scenario::inputSample (id, n, startSample + s, cfg.sr);
        }

        nodeOut.clear();   // contract: outputs cleared before call (ReverbAlgorithm.h:59-65)
        algo->processBlock (nodeIn, nodeOut, cfg.block);

        for (int n = 0; n < nodes; ++n)
            std::memcpy (out[static_cast<size_t> (n)].data() + startSample,
                         nodeOut.getReadPointer (n),
                         static_cast<size_t> (cfg.block) * sizeof (float));
        gBench.blockEnd (b, -1.0);
    }

    return out;
}

//==============================================================================
// Effects: spatcore/effects, driven synchronously on this thread. Modules are
// mono and in-place, so one scenario renders cfg.numIn INDEPENDENT chains —
// one per input stream, each prepared with its own ChainConfig::noiseKey, so
// the per-channel keyed noise (bitcrusher dither, modulation/phaser random
// LFOs, the reverb's node identity) is part of the hash rather than eleven
// copies of channel 0.
//
// A module scenario drives its module through a ModuleSlot, not bare: the
// bypass crossfade, the reset-at-silence and commitPendingVariant() all live
// in the slot, and those are precisely what the scripted timeline toggles. The
// chain scenario drives an EffectChain, which additionally owns the reorder
// envelope and the chain bypass/mute envelopes.
//
// Parameters step at the same 50 Hz tick cadence as every other path
// (scenario::effectsParams). EffectChain re-reads them only when
// params.revision moves, which effectsParams bumps on every tick.
//==============================================================================
ChannelData renderEffects (scenario::Id id, const Config& cfg)
{
    using namespace spatcore::effects;

    // The self-test rejects three different faults in the chain-order data and
    // all three make the reorder gate inert; print which one it actually found
    // rather than leaving the reader hunting a typo that may not be there.
    if (const auto why = scenario::effectsSelfTestFailure(); ! why.empty())
    {
        std::fprintf (stderr,
            "FATAL: effects scenario self-test failed - the reorder gate would "
            "be inert: %s\n", why.c_str());
        std::exit (5);
    }

    const int srInt = static_cast<int> (cfg.sr);
    const int numChains = cfg.numIn;
    const bool wholeChain = (id == scenario::Id::FxChain);
    const int slotIndex = scenario::effectsSlotIndex (id);

    if (! wholeChain && (slotIndex < 0 || slotIndex >= kNumModuleSlots))
    {
        std::fprintf (stderr, "FATAL: '%s' is not an effects scenario\n", scenario::name (id));
        std::exit (2);
    }

    const ModuleId moduleType = wholeChain ? ModuleId::Count : kSlots[slotIndex].type;
    const int moduleInstance = wholeChain ? 0 : static_cast<int> (kSlots[slotIndex].instance);

    // Tick 0 parameters: for a module slot this FIRST applyParams after
    // prepare() is what snaps the module's smoothers to their targets, so a
    // render starts settled instead of gliding in from the defaults.
    EffectChannelParams params = scenario::effectsParams (id, 0);

    std::vector<std::unique_ptr<EffectChain>> chains;
    std::vector<std::unique_ptr<ModuleSlot>> slots;   // atomics inside: held by pointer

    for (int c = 0; c < numChains; ++c)
    {
        ChainConfig chainCfg;
        chainCfg.sampleRate = cfg.sr;
        chainCfg.maxBlock = cfg.block;
        chainCfg.noiseKey = static_cast<std::uint32_t> (c) + 1u;

        if (wholeChain)
        {
            auto chain = std::make_unique<EffectChain>();
            chain->prepare (chainCfg);
            chains.push_back (std::move (chain));
        }
        else
        {
            auto slot = std::make_unique<ModuleSlot>();
            slot->prepare (chainCfg, createModule (moduleType, moduleInstance, chainCfg));
            if (! slot->hasModule())
            {
                std::fprintf (stderr, "FATAL: no module for scenario '%s'\n", scenario::name (id));
                std::exit (2);
            }
            slot->applyParams (params, moduleInstance);
            slots.push_back (std::move (slot));
        }
    }

    const int64_t total = static_cast<int64_t> (cfg.blocks) * cfg.block;
    ChannelData out (static_cast<size_t> (numChains),
                     std::vector<float> (static_cast<size_t> (total), 0.0f));

    std::vector<float> buf (static_cast<size_t> (cfg.block));
    int lastTick = 0;   // tick 0 already applied

    for (int b = 0; b < cfg.blocks; ++b)
    {
        gBench.blockBegin (b);
        const int64_t startSample = static_cast<int64_t> (b) * cfg.block;

        // Parameter timeline: re-cook between blocks at tick boundaries, the
        // same cadence the app's 50 Hz publisher uses.
        const int tick = tickForSample (startSample, srInt);
        if (tick != lastTick)
        {
            params = scenario::effectsParams (id, tick);
            for (auto& slot : slots)
                slot->applyParams (params, moduleInstance);
            lastTick = tick;
        }

        for (int c = 0; c < numChains; ++c)
        {
            for (int s = 0; s < cfg.block; ++s)
                buf[static_cast<size_t> (s)] =
                    scenario::inputSample (id, c, startSample + s, cfg.sr);

            if (wholeChain)
                chains[static_cast<size_t> (c)]->process (buf.data(), cfg.block, params);
            else
                slots[static_cast<size_t> (c)]->process (buf.data(), cfg.block);

            std::memcpy (out[static_cast<size_t> (c)].data() + startSample,
                         buf.data(), static_cast<size_t> (cfg.block) * sizeof (float));
        }
        gBench.blockEnd (b, -1.0);
    }

    // The slot and chain NaN traps silence and reset whatever produced a
    // non-finite sample. That is deterministic, so a tripped render still
    // hashes — and would gate the trap instead of the module. Say so loudly.
    std::uint32_t nanTrips = 0, silentResets = 0;
    for (auto& chain : chains)
    {
        nanTrips += chain->nanTrips.load();
        for (int k = 0; k < kNumModuleSlots; ++k)
        {
            nanTrips += chain->getSlot (k).nanTrips.load();
            silentResets += chain->getSlot (k).silentResets.load();
        }
    }
    for (auto& slot : slots)
    {
        nanTrips += slot->nanTrips.load();
        silentResets += slot->silentResets.load();
    }

    std::fprintf (stderr,
                  "%s effects/%s: chains=%d nanTrips=%u silentResets=%u\n",
                  nanTrips != 0 ? "WARNING:" : "note:",
                  scenario::name (id), numChains, nanTrips, silentResets);

    // A gate whose own safety net fired is not a gate: the hash is then the
    // trap's output, not the module's. main() turns any non-zero total into
    // exit 8 and refuses to check or record a baseline from such a run.
    gNanTripTotal += nanTrips;

    return out;
}

//==============================================================================
// GPU gather / scatter (milestone 2): synchronous backend drive per the design
// doc — makeWfsBackend/makeObBackend(deviceId) -> prepare(..., latency 0, ...)
// -> setMatrixPointers -> processBlock. With pipelineLatencyMs = 0 there is no
// -L delay pre-subtraction and no primed silence. NativeGpu*Algorithm and
// GpuAsyncPipelineT are bypassed entirely.
//==============================================================================
#if WFS_GPU_NATIVE

/** Make a plugin directory (wfs_cuda.dll + its nvrtc companions) resolvable by
    GpuBackendFactory, which falls back to LoadLibraryA/dlopen by plain name.
    Windows: prepend to PATH (additive — does not disturb PlatformDynLib's
    SetDllDirectory slot used for the HIP runtime). POSIX: dlopen does not
    re-read the environment at runtime, so the caller must launch with
    LD_LIBRARY_PATH already containing the plugin dir. */
void addPluginDirToSearchPath (const juce::File& dir)
{
#if defined(_WIN32)
    const juce::String oldPath = juce::SystemStats::getEnvironmentVariable ("PATH", {});
    const juce::String newPath = dir.getFullPathName() + ";" + oldPath;
    ::SetEnvironmentVariableA ("PATH", newPath.toRawUTF8());
#else
    std::fprintf (stderr,
        "note: on POSIX, launch with LD_LIBRARY_PATH=%s so the vendor plugin resolves\n",
        dir.getFullPathName().toRawUTF8());
#endif
}

/** Repo-relative auto-probe for the built vendor plugins: walk up from the
    harness executable looking for the app build output directories. */
juce::File autoProbePluginDir()
{
    const char* candidates[] = {
        "Builds/VisualStudio2022/x64/Release/App",
        "Builds/VisualStudio2022/x64/Debug/App",
        "Builds/LinuxMakefile/build",
    };

    auto dir = juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                   .getParentDirectory();
    for (int up = 0; up < 10 && dir != dir.getParentDirectory(); ++up)
    {
        for (const char* c : candidates)
        {
            const auto d = dir.getChildFile (c);
            if (d.getChildFile ("wfs_cuda.dll").existsAsFile()
                || d.getChildFile ("wfs_hip.dll").existsAsFile()
                || d.getChildFile ("libwfs_cuda.so").existsAsFile()
                || d.getChildFile ("libwfs_hip.so").existsAsFile())
                return d;
        }
        dir = dir.getParentDirectory();
    }
    return {};
}

/** Resolve the GPU device id: honour --device, else first enumerated GPU.
    Returns "" with a reason when unavailable. */
std::string resolveGpuDeviceId (const std::string& requested, std::string& whyNot)
{
    auto& mgr = GpuDeviceManager::instance();

    if (! requested.empty())
    {
        const GpuDevice* d = mgr.find (requested);
        if (d == nullptr)
        {
            whyNot = "device '" + requested + "' not found (available:";
            for (const auto& dev : mgr.devices())
                whyNot += " " + dev.id;
            whyNot += ")";
            return {};
        }
        if (d->isCpu())
        {
            whyNot = "'cpu' is not a GPU device";
            return {};
        }
        return requested;
    }

    const std::string first = mgr.firstGpuId();
    if (first.empty())
        whyNot = "no GPU device enumerated (vendor driver runtime absent)";
    return first;
}

/** Shared synchronous drive for both GPU paths (IWfsBackend and IObBackend
    expose the identical method surface but are unrelated types). */
template <class BackendPtr>
ChannelData renderGpuCommon (BackendPtr& backend, scenario::Id id,
                             const Config& cfg, const char* what)
{
    const int srInt = static_cast<int> (cfg.sr);
    scenario::WfsMatrices m;
    m.allocate (cfg.numIn, cfg.numOut);
    scenario::applyWfsTick (id, 0, cfg.numIn, cfg.numOut, m);

    if (! backend->prepare (cfg.numIn, cfg.numOut, cfg.block, cfg.sr,
                            /*pipelineLatencyMs*/ 0.0, /*maxDelaySeconds*/ 1.0))
    {
        std::fprintf (stderr, "FATAL: %s prepare() failed: %s\n",
                      what, backend->getLastError().c_str());
        std::exit (7);
    }
    std::fprintf (stderr, "note: %s device: %s\n", what, backend->getDeviceName().c_str());

    backend->setMatrixPointers (m.delayMs.data(), m.levels.data(), m.hfDb.data(),
                                m.frDelayMs.data(), m.frLevels.data(), m.frHfDb.data());

    const auto fr = scenario::frSettings (id);
    for (int in = 0; in < cfg.numIn; ++in)
    {
        backend->setFRFilterParams (in, fr.lowCutActive, fr.lowCutFreq,
                                    fr.highShelfActive, fr.highShelfFreq,
                                    fr.highShelfGain, fr.highShelfSlope);
        backend->setFRDiffusion (in, fr.diffusionPercent);
    }

    const int64_t total = static_cast<int64_t> (cfg.blocks) * cfg.block;
    ChannelData out (static_cast<size_t> (cfg.numOut),
                     std::vector<float> (static_cast<size_t> (total), 0.0f));

    std::vector<std::vector<float>> inBuf (static_cast<size_t> (cfg.numIn),
                                           std::vector<float> (static_cast<size_t> (cfg.block)));
    std::vector<std::vector<float>> outBuf (static_cast<size_t> (cfg.numOut),
                                            std::vector<float> (static_cast<size_t> (cfg.block)));
    std::vector<const float*> inPtrs;
    std::vector<float*> outPtrs;
    for (auto& c : inBuf)  inPtrs.push_back (c.data());
    for (auto& c : outBuf) outPtrs.push_back (c.data());

    int lastTick = 0;
    for (int b = 0; b < cfg.blocks; ++b)
    {
        gBench.blockBegin (b);
        const int64_t startSample = static_cast<int64_t> (b) * cfg.block;

        // Matrices are read at every launch; re-write them at tick boundaries
        // between the synchronous processBlock calls (same 50 Hz stepping as
        // the app's timer thread; the backend's prev->curr tracking ramps
        // per-sample inside the kernels).
        const int tick = tickForSample (startSample, srInt);
        if (tick != lastTick)
        {
            scenario::applyWfsTick (id, tick, cfg.numIn, cfg.numOut, m);
            lastTick = tick;
        }

        for (int in = 0; in < cfg.numIn; ++in)
        {
            float* dst = inBuf[static_cast<size_t> (in)].data();
            for (int s = 0; s < cfg.block; ++s)
                dst[s] = scenario::inputSample (id, in, startSample + s, cfg.sr);
        }

        if (! backend->processBlock (inPtrs.data(), outPtrs.data()))
        {
            std::fprintf (stderr, "FATAL: %s processBlock() failed on block %d: %s\n",
                          what, b, backend->getLastError().c_str());
            std::exit (7);
        }

        for (int o = 0; o < cfg.numOut; ++o)
            std::memcpy (out[static_cast<size_t> (o)].data() + startSample,
                         outBuf[static_cast<size_t> (o)].data(),
                         static_cast<size_t> (cfg.block) * sizeof (float));
        gBench.blockEnd (b, backend->getLastLaunchMs());
    }

    backend->release();
    return out;
}

ChannelData renderGpu (Path path, scenario::Id id, const Config& cfg,
                       const std::string& deviceId)
{
    if (path == Path::GpuGather)
    {
        auto b = makeWfsBackend (deviceId);
        if (b == nullptr)
        {
            std::fprintf (stderr, "FATAL: gpu-gather: could not create backend for '%s' "
                                  "(vendor plugin missing or device init failed)\n",
                          deviceId.c_str());
            std::exit (6);
        }
        return renderGpuCommon (b, id, cfg, "gpu-gather");
    }

    auto b = makeObBackend (deviceId);
    if (b == nullptr)
    {
        std::fprintf (stderr, "FATAL: gpu-scatter: could not create backend for '%s' "
                              "(vendor plugin missing or device init failed)\n",
                      deviceId.c_str());
        std::exit (6);
    }
    return renderGpuCommon (b, id, cfg, "gpu-scatter");
}

//==============================================================================
// GPU reverb (SDN / FDN / IR): synchronous backend drive, mirroring
// renderGpuCommon — makeXxxBackend(deviceId) -> prepare -> setup ->
// processBlock loop. ReverbXxxAlgorithmGPU and GpuAsyncPipelineT are bypassed
// entirely, so there is no pipeline cushion and no wet-level scaling (the
// engine applies wetLevel; the hash covers the raw backend output).
//
// Node count = --in (like the CPU reverb paths). Parameters step at the same
// 50 Hz tick cadence as the app's timer thread; the IR path stages the
// deterministic scenario IR raw (the app-side 0.125/sqrt(energy) normalisation
// lives in ReverbIRAlgorithmGPU, not in the backend under test). The IR
// backend's progressive segment load (64 segs/launch) is a pure function of
// the block sequence, so early blocks convolve against the partially loaded
// IR deterministically — hashable.
//==============================================================================
ChannelData renderGpuReverb (Path path, scenario::Id id, const Config& cfg,
                             const std::string& deviceId)
{
    const int srInt = static_cast<int> (cfg.sr);
    const int nodes = cfg.numIn;
    const char* what = pathName (path);

    auto fatalCreate = [&] ()
    {
        std::fprintf (stderr, "FATAL: %s: could not create backend for '%s' "
                              "(vendor plugin missing or device init failed)\n",
                      what, deviceId.c_str());
        std::exit (6);
    };
    auto fatalPrepare = [&] (const std::string& err)
    {
        std::fprintf (stderr, "FATAL: %s prepare() failed: %s\n", what, err.c_str());
        std::exit (7);
    };

    // Per-backend setup; the shared processBlock loop below only needs the
    // IGpuBackend surface + a per-tick parameter push (empty for IR, which
    // ignores AlgorithmParameters like its CPU counterpart).
    std::unique_ptr<ISdnBackend> sdn;
    std::unique_ptr<IFdnBackend> fdn;
    std::unique_ptr<IIrBackend> irb;
    IGpuBackend* backend = nullptr;
    std::function<void (int)> pushTickParams;

    if (path == Path::GpuReverbSdn)
    {
        sdn = makeSdnBackend (deviceId);
        if (sdn == nullptr)
            fatalCreate();
        if (! sdn->prepare (nodes, cfg.block, cfg.sr))
            fatalPrepare (sdn->getLastError());

        // Geometry first (like ReverbSDNAlgorithmGPU::prepare): inter-node
        // delays are entirely geometry-derived.
        const auto pos = scenario::nodeBox (nodes);
        std::vector<float> xyz;
        xyz.reserve (pos.size() * 3);
        for (const auto& p : pos)
        {
            xyz.push_back (p.x);
            xyz.push_back (p.y);
            xyz.push_back (p.z);
        }
        sdn->setGeometry (xyz.data(), nodes);

        pushTickParams = [&sdn, id] (int tick)
        {
            const auto p = scenario::reverbParams (id, tick);
            sdn->setParameters (p.rt60, p.rt60LowMult, p.rt60HighMult,
                                p.crossoverLow, p.crossoverHigh,
                                p.diffusion, p.sdnScale);
        };
        backend = sdn.get();
    }
    else if (path == Path::GpuReverbFdn)
    {
        fdn = makeFdnBackend (deviceId);
        if (fdn == nullptr)
            fatalCreate();
        // fdnSize is a prepare()-time constant (matching the CPU FDN whose
        // delay lengths never resize at runtime); the scenario timeline never
        // changes it (defaults to 1.0).
        if (! fdn->prepare (nodes, cfg.block, cfg.sr,
                            scenario::reverbParams (id, 0).fdnSize))
            fatalPrepare (fdn->getLastError());

        pushTickParams = [&fdn, id] (int tick)
        {
            const auto p = scenario::reverbParams (id, tick);
            fdn->setParameters (p.rt60, p.rt60LowMult, p.rt60HighMult,
                                p.crossoverLow, p.crossoverHigh, p.diffusion);
        };
        backend = fdn.get();
    }
    else // Path::GpuReverbIr
    {
        const auto ir = scenario::deterministicIr (cfg.sr);

        irb = makeIrBackend (deviceId);
        if (irb == nullptr)
            fatalCreate();
        // Device allocation sized to the scenario IR (the app caps at 10 s;
        // the harness IR is 0.5 s, keeping segCapacity — and hence the ring
        // allocation — small). Requires a power-of-two block in [4, 1024].
        if (! irb->prepare (nodes, cfg.block, cfg.sr, static_cast<int> (ir.size())))
            fatalPrepare (irb->getLastError());

        irb->stageIr (ir.data(), static_cast<int> (ir.size()));
        backend = irb.get();
    }

    std::fprintf (stderr, "note: %s device: %s\n", what, backend->getDeviceName().c_str());

    // Guarantee zeroed device state before block 0 (consumed at the first
    // processBlock), independent of whether prepare() cleared its buffers.
    if (sdn) sdn->requestReset();
    if (fdn) fdn->requestReset();
    if (irb) irb->requestReset();

    if (pushTickParams)
        pushTickParams (0);

    const int64_t total = static_cast<int64_t> (cfg.blocks) * cfg.block;
    ChannelData out (static_cast<size_t> (nodes),
                     std::vector<float> (static_cast<size_t> (total), 0.0f));

    std::vector<std::vector<float>> inBuf (static_cast<size_t> (nodes),
                                           std::vector<float> (static_cast<size_t> (cfg.block)));
    std::vector<std::vector<float>> outBuf (static_cast<size_t> (nodes),
                                            std::vector<float> (static_cast<size_t> (cfg.block)));
    std::vector<const float*> inPtrs;
    std::vector<float*> outPtrs;
    for (auto& c : inBuf)  inPtrs.push_back (c.data());
    for (auto& c : outBuf) outPtrs.push_back (c.data());

    int lastTick = 0;
    for (int b = 0; b < cfg.blocks; ++b)
    {
        gBench.blockBegin (b);
        const int64_t startSample = static_cast<int64_t> (b) * cfg.block;

        const int tick = tickForSample (startSample, srInt);
        if (tick != lastTick)
        {
            if (pushTickParams)
                pushTickParams (tick);
            lastTick = tick;
        }

        for (int n = 0; n < nodes; ++n)
        {
            float* dst = inBuf[static_cast<size_t> (n)].data();
            for (int s = 0; s < cfg.block; ++s)
                dst[s] = scenario::inputSample (id, n, startSample + s, cfg.sr);
        }

        if (! backend->processBlock (inPtrs.data(), outPtrs.data()))
        {
            std::fprintf (stderr, "FATAL: %s processBlock() failed on block %d: %s\n",
                          what, b, backend->getLastError().c_str());
            std::exit (7);
        }

        for (int n = 0; n < nodes; ++n)
            std::memcpy (out[static_cast<size_t> (n)].data() + startSample,
                         outBuf[static_cast<size_t> (n)].data(),
                         static_cast<size_t> (cfg.block) * sizeof (float));
        gBench.blockEnd (b, backend->getLastLaunchMs());
    }

    backend->release();
    return out;
}

#endif // WFS_GPU_NATIVE

//==============================================================================
ChannelData renderOne (Path path, scenario::Id id, const Config& cfg,
                       const std::string& gpuDeviceId)
{
    switch (path)
    {
        case Path::CpuGather:  return renderCpuGather (id, cfg);
        case Path::CpuScatter: return renderCpuScatter (id, cfg);
        case Path::ReverbSdn:
        case Path::ReverbFdn:
        case Path::ReverbIr:   return renderReverb (path, id, cfg);
        case Path::Effects:    return renderEffects (id, cfg);
        case Path::GpuGather:
        case Path::GpuScatter:
#if WFS_GPU_NATIVE
            return renderGpu (path, id, cfg, gpuDeviceId);
#else
            break;
#endif
        case Path::GpuReverbSdn:
        case Path::GpuReverbFdn:
        case Path::GpuReverbIr:
#if WFS_GPU_NATIVE
            return renderGpuReverb (path, id, cfg, gpuDeviceId);
#else
            break;
#endif
    }
#if ! WFS_GPU_NATIVE
    (void) gpuDeviceId;
    std::fprintf (stderr, "FATAL: GPU paths require a WFS_GPU_NATIVE build\n");
    std::exit (6);
#endif
    return {};
}

/** SHA-256 of the raw float32 PCM: all output channels, channel-major,
    little-endian byte dump (matches `sha256sum` of the --raw file). */
std::string hashChannels (const ChannelData& chans)
{
    orh::Sha256 sha;
    for (const auto& c : chans)
        sha.update (c.data(), c.size() * sizeof (float));
    return sha.finalHex();
}

bool writeRaw (const juce::File& f, const ChannelData& chans)
{
    f.deleteFile();
    juce::FileOutputStream os (f);
    if (! os.openedOk())
        return false;
    for (const auto& c : chans)
        os.write (c.data(), c.size() * sizeof (float));
    return true;
}

bool writeWav (const juce::File& f, const ChannelData& chans, double sr)
{
    if (chans.empty())
        return false;

    f.deleteFile();
    auto os = std::make_unique<juce::FileOutputStream> (f);
    if (! os->openedOk())
        return false;

    juce::WavAudioFormat fmt;
    std::unique_ptr<juce::AudioFormatWriter> writer (
        fmt.createWriterFor (os.get(), sr, static_cast<unsigned int> (chans.size()),
                             32, {}, 0));
    if (writer == nullptr)
        return false;
    os.release();   // writer owns the stream now

    std::vector<const float*> ptrs;
    for (const auto& c : chans)
        ptrs.push_back (c.data());
    return writer->writeFromFloatArrays (ptrs.data(), static_cast<int> (chans.size()),
                                         static_cast<int> (chans[0].size()));
}

/** Insert "<tag>." before the file extension for multi-combo output files. */
juce::File taggedFile (const juce::File& base, const std::string& tag)
{
    return base.getSiblingFile (base.getFileNameWithoutExtension()
                                + "." + juce::String (tag) + base.getFileExtension());
}

void usage()
{
    std::fprintf (stderr,
        "usage: offline-render --path <cpu-gather|cpu-scatter|reverb-sdn|reverb-fdn|reverb-ir\n"
        "                              |effects|gpu-gather|gpu-scatter|gpu-reverb-sdn\n"
        "                              |gpu-reverb-fdn|gpu-reverb-ir|cpu|gpu|all>\n"
        "                      --scenario <static|moving|fr-toggle|stereo|all>   (render paths)\n"
        "                      --scenario <dist|eq|dyn|mod|phaser|trem|reverb|delay|crush\n"
        "                                  |chain|all>                           (--path effects)\n"
        "                      [--stereo-null]\n"
        "                      [--blocks N] [--block 512] [--sr 48000] [--in 8] [--out 16]\n"
        "                      [--device cuda:0] [--plugin-dir <dir with wfs_cuda.dll>]\n"
        "                      [--wav out.wav] [--raw out.f32]\n"
        "                      [--check baselines/<machine>.json] [--update]\n"
        "                      [--bench] [--warmup 16] [--bench-json <file>]\n"
        "\n"
        "GPU baselines are per device+driver: keep them in a separate file and check\n"
        "them in a separate invocation, e.g.\n"
        "  offline-render --path cpu     --check baselines/<machine>.json\n"
        "  offline-render --path effects --check baselines/<machine>.json\n"
        "  offline-render --path gpu     --check baselines/<machine>-gpu.json\n"
        "\n"
        "--check and --update apply at the DEFAULT render shape only, and the shape\n"
        "is stamped into the baseline file under the reserved \"#shape\" key. Any other\n"
        "--blocks/--block/--sr/--in/--out is refused with exit 2 rather than compared:\n"
        "a short run stops part-way through a scenario script, so recording it would\n"
        "produce a golden that gates only the part it reached.\n"
        "\n"
        "--path effects renders spatcore/effects: one scenario per module through a\n"
        "ModuleSlot (bypass toggle, parameter sweeps, variant switch), plus a chain\n"
        "scenario that reorders the chain and toggles chain bypass and mute. It uses\n"
        "--in as the number of independent mono chains and ignores --out. Its hashes\n"
        "live in the CPU baseline file but are still per-machine: the modules call\n"
        "std::tanh/std::cos/std::exp, so they must be recorded on each machine.\n"
        "\n"
        "--stereo-null renders the Phase-0 null pair on the WFS paths (gather/scatter)\n"
        "and compares the two hashes against EACH OTHER instead of a baseline: a\n"
        "width-0 stereo channel (silent centre on slot 0, L/R on 1/2, 3..5 claimed-\n"
        "and-silent) must be bit-identical to two mono channels at the same position.\n"
        "exit 1 on mismatch.\n"
        "\n"
        "--bench reports blocks / wall ms / xRealtime / budget ms per combo (plus the\n"
        "launchMs min/med/p99/max/mean distribution on GPU paths), excluding the first\n"
        "--warmup blocks. Bench shapes other than the default are not baselined.\n"
        "\n"
        "exit codes: 0 ok, 1 baseline mismatch, 2 usage or a refused invocation,\n"
        "            3 drain timeout, 4 IR-load timeout, 5 self-test,\n"
        "            6 GPU/plugin unavailable, 7 GPU runtime failure,\n"
        "            8 a NaN trap tripped during a render\n");
}

} // namespace

//==============================================================================
int main (int argc, char* argv[])
{
    if (! orh::Sha256::selfTest())
    {
        std::fprintf (stderr, "FATAL: SHA-256 self-test failed\n");
        return 5;
    }
    if (juce::ByteOrder::isBigEndian())
    {
        std::fprintf (stderr, "FATAL: hashes are defined on little-endian float dumps\n");
        return 5;
    }

    Config cfg;
    std::string pathArg = "all", scenarioArg = "all";
    std::string wavArg, rawArg, checkArg, deviceArg, pluginDirArg, benchJsonArg;
    bool update = false;
    bool stereoNull = false;

    for (int i = 1; i < argc; ++i)
    {
        const std::string a = argv[i];
        auto next = [&] () -> std::string
        {
            if (i + 1 >= argc)
            {
                std::fprintf (stderr, "error: %s needs a value\n", a.c_str());
                usage();
                std::exit (2);
            }
            return argv[++i];
        };

        if      (a == "--path")     pathArg = next();
        else if (a == "--scenario") scenarioArg = next();
        else if (a == "--blocks")   cfg.blocks = std::atoi (next().c_str());
        else if (a == "--block")    cfg.block = std::atoi (next().c_str());
        else if (a == "--sr")       cfg.sr = std::atof (next().c_str());
        else if (a == "--in")       cfg.numIn = std::atoi (next().c_str());
        else if (a == "--out")      cfg.numOut = std::atoi (next().c_str());
        else if (a == "--reverb-workers") cfg.reverbWorkers = std::atoi (next().c_str());
        else if (a == "--device")   deviceArg = next();
        else if (a == "--plugin-dir") pluginDirArg = next();
        else if (a == "--wav")      wavArg = next();
        else if (a == "--raw")      rawArg = next();
        else if (a == "--check")    checkArg = next();
        else if (a == "--update")   update = true;
        else if (a == "--stereo-null") stereoNull = true;
        else if (a == "--bench")    gBench.enabled = true;
        else if (a == "--warmup")   gBench.warmup = std::atoi (next().c_str());
        else if (a == "--bench-json") { benchJsonArg = next(); gBench.enabled = true; }
        else if (a == "--help" || a == "-h") { usage(); return 0; }
        else
        {
            std::fprintf (stderr, "error: unknown argument '%s'\n", a.c_str());
            usage();
            return 2;
        }
    }

    if (cfg.blocks <= 0 || cfg.block <= 0 || cfg.sr <= 0.0
        || cfg.numIn <= 0 || cfg.numOut <= 0)
    {
        std::fprintf (stderr, "error: invalid size/rate arguments\n");
        return 2;
    }
    if (gBench.warmup < 0)
    {
        std::fprintf (stderr, "error: --warmup must be >= 0\n");
        return 2;
    }

    std::vector<Path> paths;
    bool gpuOptional = false;   // --path all: skip gpu paths with a note when unavailable
    if (pathArg == "all")
    {
        paths = cpuOnlyPaths();
        for (const Path p : gpuPaths())
            paths.push_back (p);
        gpuOptional = true;
    }
    else if (pathArg == "cpu")
        paths = cpuPaths();
    else if (pathArg == "gpu")
        paths = gpuPaths();
    else
    {
        Path p;
        if (! pathFromName (pathArg, p))
        {
            std::fprintf (stderr, "error: unknown path '%s'\n", pathArg.c_str());
            return 2;
        }
        paths.push_back (p);
    }

    // Scenario families do not cross: the WFS/reverb render paths take the four
    // matrix timelines, --path effects takes the ten module timelines. With an
    // explicit --scenario, the paths of the OTHER family are dropped, so
    // "--path all --scenario static" still means exactly what it used to.
    const bool allScenarios = (scenarioArg == "all");
    scenario::Id namedScenario = scenario::Id::Static;

    if (! allScenarios && ! scenario::fromName (scenarioArg, namedScenario))
    {
        std::fprintf (stderr, "error: unknown scenario '%s'\n", scenarioArg.c_str());
        return 2;
    }

    if (! allScenarios)
    {
        const bool wantsEffects = scenario::isEffectsScenario (namedScenario);
        paths.erase (std::remove_if (paths.begin(), paths.end(),
                                     [wantsEffects] (Path p)
                                     { return (p == Path::Effects) != wantsEffects; }),
                     paths.end());

        if (paths.empty())
        {
            std::fprintf (stderr,
                "error: scenario '%s' is %san effects scenario — it only runs on %s\n",
                scenarioArg.c_str(), wantsEffects ? "" : "not ",
                wantsEffects ? "--path effects" : "the WFS/reverb render paths");
            return 2;
        }
    }

    auto scenariosFor = [&] (Path p) -> std::vector<scenario::Id>
    {
        if (! allScenarios)
            return { namedScenario };
        return (p == Path::Effects) ? scenario::allEffectsScenarios()
                                    : scenario::allScenarios();
    };

    // CPU workers consume fixed 64-sample sub-blocks; a non-multiple block size
    // would leave a residue in the input rings and stall the drain forever.
    const bool hasCpuPath = std::any_of (paths.begin(), paths.end(), [] (Path p)
                                { return p == Path::CpuGather || p == Path::CpuScatter; });
    if (hasCpuPath && (cfg.block % 64) != 0)
    {
        std::fprintf (stderr, "error: --block must be a multiple of 64 for the CPU paths\n");
        return 2;
    }

    //==========================================================================
    // GPU availability: resolve the device and plugin BEFORE rendering so
    // --path all can skip cleanly on GPU-less machines (the CPU baseline gate
    // must still pass there), while explicit gpu requests fail loudly (exit 6).
    //==========================================================================
    std::string gpuDeviceId;
    const bool wantsGpu = std::any_of (paths.begin(), paths.end(), isGpuPath);
    if (wantsGpu)
    {
        std::string whyNot;
        bool gpuOk = false;

#if WFS_GPU_NATIVE
        // Make the vendor plugin dir resolvable before the factory's dlopen.
        juce::File pluginDir;
        if (! pluginDirArg.empty())
            pluginDir = juce::File::getCurrentWorkingDirectory()
                            .getChildFile (juce::String (pluginDirArg));
        else
            pluginDir = autoProbePluginDir();

        if (pluginDir.isDirectory())
        {
            addPluginDirToSearchPath (pluginDir);
            std::fprintf (stderr, "note: GPU plugin dir: %s\n",
                          pluginDir.getFullPathName().toRawUTF8());
        }

        gpuDeviceId = resolveGpuDeviceId (deviceArg, whyNot);
        if (! gpuDeviceId.empty())
        {
            auto probe = makeWfsBackend (gpuDeviceId);   // dlopen + create only
            if (probe != nullptr)
                gpuOk = true;
            else
                whyNot = "vendor plugin for '" + gpuDeviceId
                       + "' not found/loadable (see --plugin-dir)";
        }
#else
        whyNot = "harness built without WFS_GPU_NATIVE";
#endif

        if (! gpuOk)
        {
            if (gpuOptional)
            {
                std::fprintf (stderr, "note: skipping gpu paths: %s\n", whyNot.c_str());
                paths.erase (std::remove_if (paths.begin(), paths.end(), isGpuPath),
                             paths.end());
            }
            else
            {
                std::fprintf (stderr, "error: GPU unavailable: %s\n", whyNot.c_str());
                return 6;
            }
        }
    }

    //==========================================================================
    // Phase-0 stereo null test (handoff doc §8): a width-0 stereo channel —
    // six render sources: silent centre on slot 0, L/R on slots 1/2, slots
    // 3..5 claimed-and-silent — must render BIT-IDENTICAL to two mono
    // channels at the same position.
    // Self-referential (hash vs hash), no baseline file involved. Runs on the
    // WFS render paths only: the reverb paths have no per-source slot
    // semantics of their own.
    //==========================================================================
    if (stereoNull)
    {
        std::vector<Path> nullPaths;
        for (const Path p : paths)
            if (p == Path::CpuGather || p == Path::CpuScatter
                || p == Path::GpuGather || p == Path::GpuScatter)
                nullPaths.push_back (p);

        if (nullPaths.empty())
        {
            std::fprintf (stderr, "error: --stereo-null needs at least one WFS path "
                                  "(cpu-gather/cpu-scatter/gpu-gather/gpu-scatter)\n");
            return 2;
        }

        bool allMatch = true;
        for (const Path p : nullPaths)
        {
            Config cfgStereo = cfg;
            cfgStereo.numIn = 6;
            Config cfgMono = cfg;
            cfgMono.numIn = 2;

            const std::string hashStereo =
                hashChannels (renderOne (p, scenario::Id::StereoNull, cfgStereo, gpuDeviceId));
            const std::string hashMono =
                hashChannels (renderOne (p, scenario::Id::StereoNullMono, cfgMono, gpuDeviceId));

            const bool match = hashStereo == hashMono;
            std::printf ("%s stereo-null %s\n  stereo    sha256=%s\n  dual-mono sha256=%s\n",
                         pathName (p), match ? "OK" : "MISMATCH",
                         hashStereo.c_str(), hashMono.c_str());
            std::fflush (stdout);
            allMatch = allMatch && match;
        }

        if (allMatch)
            std::printf ("stereo-null check OK (%d path%s)\n",
                         (int) nullPaths.size(), nullPaths.size() == 1 ? "" : "s");
        return allMatch ? 0 : 1;
    }

    size_t comboCount = 0;
    for (const Path p : paths)
        comboCount += scenariosFor (p).size();

    const bool multiCombo = comboCount > 1;
    std::map<std::string, std::string> results;   // "path/scenario" -> sha256

    for (const Path p : paths)
    {
        for (const scenario::Id s : scenariosFor (p))
        {
            const std::string key = std::string (pathName (p)) + "/" + scenario::name (s);
            gBench.beginCombo (cfg);
            const ChannelData chans = renderOne (p, s, cfg, gpuDeviceId);
            const std::string hash = hashChannels (chans);
            results[key] = hash;
            std::printf ("%s sha256=%s\n", key.c_str(), hash.c_str());
            std::fflush (stdout);
            gBench.report (key, cfg);

            const std::string tag = std::string (pathName (p)) + "-" + scenario::name (s);
            if (! wavArg.empty())
            {
                auto f = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String (wavArg));
                if (multiCombo) f = taggedFile (f, tag);
                if (! writeWav (f, chans, cfg.sr))
                    std::fprintf (stderr, "warning: could not write %s\n",
                                  f.getFullPathName().toRawUTF8());
            }
            if (! rawArg.empty())
            {
                auto f = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String (rawArg));
                if (multiCombo) f = taggedFile (f, tag);
                if (! writeRaw (f, chans))
                    std::fprintf (stderr, "warning: could not write %s\n",
                                  f.getFullPathName().toRawUTF8());
            }
        }
    }

    if (! benchJsonArg.empty())
    {
        const auto f = juce::File::getCurrentWorkingDirectory()
                           .getChildFile (juce::String (benchJsonArg));
        if (gBench.writeJson (f, cfg))
            std::fprintf (stderr, "note: bench JSON written to %s\n",
                          f.getFullPathName().toRawUTF8());
        else
            std::fprintf (stderr, "warning: could not write %s\n",
                          f.getFullPathName().toRawUTF8());
    }

    // A render whose NaN trap fired hashes the trap, not the module - so it is
    // not a gate. Fail here, before any baseline is consulted or written, so a
    // tripped run can neither pass --check nor be recorded by --update. The
    // per-render WARNING above says which scenario it was.
    if (gNanTripTotal != 0)
    {
        std::fprintf (stderr,
            "FATAL: %u NaN trap(s) tripped during this run — the hash is the "
            "trap's output, not the module's. Refusing to check or record "
            "a baseline.\n", gNanTripTotal);
        return 8;
    }

    if (checkArg.empty())
        return 0;

    //==========================================================================
    // Baseline check / update (same contract as tools/validation/kernel_hashes.py)
    //==========================================================================
    auto baselineFile = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String (checkArg));

    // One read of the file, shared by the shape guard, the merge and the check.
    std::map<std::string, std::string> recorded;
    if (baselineFile.existsAsFile())
    {
        const auto parsed = juce::JSON::parse (baselineFile.loadFileAsString());
        if (auto* obj = parsed.getDynamicObject())
            for (const auto& prop : obj->getProperties())
                recorded[prop.name.toString().toStdString()] =
                    prop.value.toString().toStdString();
    }

    //==========================================================================
    // Render-shape guard (kShapeKey). A hash is only comparable to another one
    // rendered at the same shape, and only the DEFAULT shape is ever baselined
    // - --bench shapes explicitly are not. Both --check and --update are
    // refused off-shape rather than one of them being trusted to be loud:
    // --check would MISMATCH, but --update would silently record a golden that
    // gates a fraction of the script (a 30-block effects run never reaches the
    // end of the bypass window, let alone either variant switch) and exit 0.
    //==========================================================================
    const std::string runShape = shapeString (cfg);
    {
        const Config defaults;
        const std::string defaultShape = shapeString (defaults);

        if (runShape != defaultShape)
        {
            std::fprintf (stderr,
                "error: --check/--update apply at the default render shape only\n"
                "       this run:  %s\n"
                "       baselined: %s\n"
                "       drop --check to render this shape anyway (hashes still print)\n",
                runShape.c_str(), defaultShape.c_str());
            return 2;
        }

        const auto it = recorded.find (kShapeKey);

        if (it != recorded.end() && it->second != runShape)
        {
            std::fprintf (stderr,
                "error: %s was recorded at a different render shape, so every entry\n"
                "       in it is stale\n"
                "       recorded: %s\n"
                "       this run: %s\n"
                "       delete the file and re-record each path with --update\n",
                baselineFile.getFileName().toRawUTF8(),
                it->second.c_str(), runShape.c_str());
            return 2;
        }
    }

    if (update)
    {
        // Merge: keep entries for combos not rendered in this invocation.
        std::map<std::string, std::string> merged = recorded;
        for (const auto& r : results)
            merged[r.first] = r.second;

        merged[kShapeKey] = runShape;   // stamped on every write, old files included

        juce::String json = "{\n";
        size_t i = 0;
        for (const auto& e : merged)
        {
            json << "  \"" << juce::String (e.first) << "\": \""
                 << juce::String (e.second) << "\"";
            if (++i < merged.size()) json << ",";
            json << "\n";
        }
        json << "}\n";

        baselineFile.getParentDirectory().createDirectory();
        if (! baselineFile.replaceWithText (json))
        {
            std::fprintf (stderr, "error: could not write %s\n",
                          baselineFile.getFullPathName().toRawUTF8());
            return 2;
        }
        std::printf ("wrote %s (%d hash entries, shape %s)\n",
                     baselineFile.getFullPathName().toRawUTF8(),
                     static_cast<int> (merged.size() - merged.count (kShapeKey)),
                     runShape.c_str());
        return 0;
    }

    if (! baselineFile.existsAsFile())
    {
        std::fprintf (stderr, "error: baseline %s missing — run with --update to create it\n",
                      baselineFile.getFullPathName().toRawUTF8());
        return 1;
    }

    const std::map<std::string, std::string>& expected = recorded;

    std::vector<std::string> problems;
    for (const auto& r : results)
    {
        auto it = expected.find (r.first);
        if (it == expected.end())
            problems.push_back ("MISSING   " + r.first + " (not in baseline — run --update if intentional)");
        else if (it->second != r.second)
            problems.push_back ("MISMATCH  " + r.first + "\n    expected " + it->second
                                + "\n    actual   " + r.second);
    }

    if (! problems.empty())
    {
        std::printf ("offline-render baseline check FAILED:\n");
        for (const auto& p : problems)
            std::printf ("  %s\n", p.c_str());
        return 1;
    }

    std::printf ("offline-render baseline check OK (%d combos match %s)\n",
                 static_cast<int> (results.size()),
                 baselineFile.getFileName().toRawUTF8());
    return 0;
}
