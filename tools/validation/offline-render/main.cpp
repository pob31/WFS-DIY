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
//                          |gpu-gather|gpu-scatter|gpu-reverb-sdn|gpu-reverb-fdn
//                          |gpu-reverb-ir|cpu|gpu|all>
//                  --scenario <static|moving|fr-toggle|all>
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

struct Config
{
    double sr = 48000.0;
    int block = 512;
    int blocks = 200;
    int numIn = 8;
    int numOut = 16;
};

enum class Path
{
    CpuGather,
    CpuScatter,
    ReverbSdn,
    ReverbFdn,
    ReverbIr,
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

    AudioParallelFor pool;
    pool.prepare (0);   // 0 workers -> sequential fallback (worker-count-invariant anyway)

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
        "                              |gpu-gather|gpu-scatter|gpu-reverb-sdn|gpu-reverb-fdn\n"
        "                              |gpu-reverb-ir|cpu|gpu|all>\n"
        "                      --scenario <static|moving|fr-toggle|all>\n"
        "                      [--blocks N] [--block 512] [--sr 48000] [--in 8] [--out 16]\n"
        "                      [--device cuda:0] [--plugin-dir <dir with wfs_cuda.dll>]\n"
        "                      [--wav out.wav] [--raw out.f32]\n"
        "                      [--check baselines/<machine>.json] [--update]\n"
        "                      [--bench] [--warmup 16] [--bench-json <file>]\n"
        "\n"
        "GPU baselines are per device+driver: keep them in a separate file and check\n"
        "them in a separate invocation, e.g.\n"
        "  offline-render --path cpu --check baselines/<machine>.json\n"
        "  offline-render --path gpu --check baselines/<machine>-gpu.json\n"
        "\n"
        "--bench reports blocks / wall ms / xRealtime / budget ms per combo (plus the\n"
        "launchMs min/med/p99/max/mean distribution on GPU paths), excluding the first\n"
        "--warmup blocks. Bench shapes other than the default are not baselined.\n"
        "\n"
        "exit codes: 0 ok, 1 baseline mismatch, 2 usage, 3 drain timeout,\n"
        "            4 IR-load timeout, 5 self-test, 6 GPU/plugin unavailable,\n"
        "            7 GPU runtime failure\n");
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
        else if (a == "--device")   deviceArg = next();
        else if (a == "--plugin-dir") pluginDirArg = next();
        else if (a == "--wav")      wavArg = next();
        else if (a == "--raw")      rawArg = next();
        else if (a == "--check")    checkArg = next();
        else if (a == "--update")   update = true;
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
        paths = cpuPaths();
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

    std::vector<scenario::Id> scenarios;
    if (scenarioArg == "all")
        scenarios = scenario::allScenarios();
    else
    {
        scenario::Id s;
        if (! scenario::fromName (scenarioArg, s))
        {
            std::fprintf (stderr, "error: unknown scenario '%s'\n", scenarioArg.c_str());
            return 2;
        }
        scenarios.push_back (s);
    }

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

    const bool multiCombo = paths.size() * scenarios.size() > 1;
    std::map<std::string, std::string> results;   // "path/scenario" -> sha256

    for (const Path p : paths)
    {
        for (const scenario::Id s : scenarios)
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

    if (checkArg.empty())
        return 0;

    //==========================================================================
    // Baseline check / update (same contract as tools/validation/kernel_hashes.py)
    //==========================================================================
    auto baselineFile = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String (checkArg));

    if (update)
    {
        // Merge: keep entries for combos not rendered in this invocation.
        std::map<std::string, std::string> merged;
        if (baselineFile.existsAsFile())
        {
            const auto parsed = juce::JSON::parse (baselineFile.loadFileAsString());
            if (auto* obj = parsed.getDynamicObject())
                for (const auto& prop : obj->getProperties())
                    merged[prop.name.toString().toStdString()] =
                        prop.value.toString().toStdString();
        }
        for (const auto& r : results)
            merged[r.first] = r.second;

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
        std::printf ("wrote %s (%d entries)\n",
                     baselineFile.getFullPathName().toRawUTF8(),
                     static_cast<int> (merged.size()));
        return 0;
    }

    if (! baselineFile.existsAsFile())
    {
        std::fprintf (stderr, "error: baseline %s missing — run with --update to create it\n",
                      baselineFile.getFullPathName().toRawUTF8());
        return 1;
    }

    std::map<std::string, std::string> expected;
    {
        const auto parsed = juce::JSON::parse (baselineFile.loadFileAsString());
        if (auto* obj = parsed.getDynamicObject())
            for (const auto& prop : obj->getProperties())
                expected[prop.name.toString().toStdString()] =
                    prop.value.toString().toStdString();
    }

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
