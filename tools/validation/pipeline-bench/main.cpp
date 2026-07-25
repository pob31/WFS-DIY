//==============================================================================
// pipeline-bench — real-time latency/underrun harness for the GPU async
// pipeline (companion to offline-render, which is the bit-exactness gate).
//
// offline-render drives the vendor backends SYNCHRONOUSLY with
// pipelineLatencyMs = 0 and no pump thread; nothing in the repo exercised
// GpuAsyncPipelineT (SPSC rings, pump thread, depth cushion, underrun
// accounting) until this tool. pipeline-bench instantiates the REAL pipeline
// template around the dlopen'd vendor backend and drives it from a metronomic
// simulated audio callback (waitable-timer + short spin to absolute QPC
// deadlines), exactly like the app's ASIO callback cadence.
//
//   pipeline-bench [--device cuda:0] [--plugin-dir <dir with wfs_cuda.dll>]
//                  [--sr 96000] [--block 64] [--in 64] [--out 128]
//                  [--path gather|scatter] [--scenario static|moving|fr-toggle]
//                  [--seconds 30] [--depth N | --depth-sweep A..B]
//                  [--spike-ms 1.0] [--warmup 32] [--json out.json]
//                  [--list-devices]
//
// Per depth it reports: underrun count (total / post-warmup, with first block
// indices), pump-failure status (the pump thread exits permanently on one
// backend failure and popOutput then silence-fills WITHOUT counting underruns
// — GpuAsyncPipeline.h:214-221 — so a failed run is marked FAILED, never
// clean), the per-block backend-time distribution min/med/p99/p999/max/mean
// vs the block budget, the callback wake jitter distribution (so harness
// timer noise is distinguishable from GPU stalls), and a wall-clock
// timestamped spike log (backend calls exceeding --spike-ms) — erratic
// multi-ms spikes clustered in time on the WDDM display GPU but absent on an
// MCDM compute GPU are the desktop-compositor stall signature.
//
// The matrix arrays are rewritten at 50 Hz tick boundaries on the callback
// thread while the pump thread snapshots them — the same benign torn-float
// race the app accepts by design. Default scenario is `moving` because the
// backend's upload change-detection skips all matrix H2D when matrices are
// static, which understates the real per-block cost.
//==============================================================================

#include <JuceHeader.h>

#if defined(_WIN32)
 #define WIN32_LEAN_AND_MEAN
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <timeapi.h>
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "../../../spatcore/gpu/GpuDeviceManager.h"
#include "../../../spatcore/gpu/GpuBackendInterface.h"
#include "../../../spatcore/gpu/WfsGpuBackend.h"      // makeWfsBackend (gather)
#include "../../../spatcore/gpu/ObGpuBackend.h"       // makeObBackend  (scatter)
#include "../../../spatcore/gpu/GpuAsyncPipeline.h"
#include "../../../spatcore/rt/RtThreadPriority.h"

#include "../offline-render/scenarios.h"

namespace
{

//==============================================================================
double nowMs()
{
    using namespace std::chrono;
    return duration<double, std::milli> (steady_clock::now().time_since_epoch()).count();
}

//==============================================================================
struct Config
{
    double sr = 96000.0;
    int block = 64;
    int numIn = 64;
    int numOut = 128;
    double seconds = 30.0;
    int warmup = 32;                 // blocks excluded from distributions
    double spikeMs = 1.0;
    bool scatter = false;            // false = gather (WFS), true = scatter (OB)
    scenario::Id scenarioId = scenario::Id::Moving;
    std::string deviceArg, pluginDirArg, jsonArg;
    std::vector<int> depths { 1, 2, 3, 4, 5, 6, 7, 8 };

    double blockMs() const { return 1000.0 * block / sr; }
    int totalBlocks() const { return (int) std::ceil (seconds * sr / block); }
};

//==============================================================================
// InstrumentedBackend — decorator between GpuAsyncPipelineT and the dlopen'd
// vendor backend. Satisfies the pipeline's backend contract subset
// (processBlock / isReady / getLastError / getLastLaunchMs) and records a
// {relative wall time, duration} sample per call into a preallocated buffer
// (pump thread is the only writer; no allocation in the hot path).
//==============================================================================
struct InstrumentedBackend
{
    spatcore::gpu::IGpuBackend* inner = nullptr;

    struct Sample { double tRelMs; double durMs; };
    std::vector<Sample> samples;
    std::atomic<size_t> count { 0 };
    double epochMs = 0.0;

    void begin (spatcore::gpu::IGpuBackend* b, size_t capacity)
    {
        inner = b;
        samples.assign (capacity, {});
        count.store (0, std::memory_order_relaxed);
        epochMs = nowMs();
    }

    bool processBlock (const float* const* in, float* const* out)
    {
        const double s = nowMs();
        const bool ok = inner->processBlock (in, out);
        const double e = nowMs();

        const size_t i = count.load (std::memory_order_relaxed);
        if (i < samples.size())
            samples[i] = { s - epochMs, e - s };
        count.store (i + 1, std::memory_order_release);
        return ok;
    }

    bool isReady() const noexcept                      { return inner != nullptr && inner->isReady(); }
    const std::string& getLastError() const noexcept   { return inner->getLastError(); }
    double getLastLaunchMs() const noexcept            { return inner->getLastLaunchMs(); }
};

//==============================================================================
// Metronome — absolute-deadline block clock for the simulated audio callback.
// Windows: high-resolution waitable timer to (deadline - spin margin), then a
// short spin on the steady clock. Elsewhere: sleep_until + spin.
//==============================================================================
class Metronome
{
public:
    Metronome()
    {
#if defined(_WIN32)
       #ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
        #define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
       #endif
        timer = ::CreateWaitableTimerExW (nullptr, nullptr,
                                          CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                          TIMER_ALL_ACCESS);
        if (timer == nullptr)   // pre-1803 fallback: plain waitable timer
            timer = ::CreateWaitableTimerW (nullptr, FALSE, nullptr);
#endif
    }

    ~Metronome()
    {
#if defined(_WIN32)
        if (timer != nullptr)
            ::CloseHandle (timer);
#endif
    }

    /** Blocks until the absolute steady-clock deadline (ms). Returns the
        overshoot (wake - deadline) in ms, >= 0 apart from clock granularity. */
    double waitUntil (double deadlineMs) const
    {
        constexpr double spinMarginMs = 0.2;

        for (;;)
        {
            const double remaining = deadlineMs - nowMs();
            if (remaining <= spinMarginMs)
                break;

#if defined(_WIN32)
            if (timer != nullptr)
            {
                LARGE_INTEGER due;
                due.QuadPart = -(LONGLONG) ((remaining - spinMarginMs) * 10000.0); // 100 ns units, relative
                if (::SetWaitableTimer (timer, &due, 0, nullptr, nullptr, FALSE))
                {
                    ::WaitForSingleObject (timer, (DWORD) (remaining + 5.0));
                    continue;
                }
            }
            ::Sleep (0);
#else
            std::this_thread::sleep_for (
                std::chrono::duration<double, std::milli> (remaining - spinMarginMs));
#endif
        }

        while (nowMs() < deadlineMs)
            { /* spin the last ~200 us to the deadline */ }

        return nowMs() - deadlineMs;
    }

private:
#if defined(_WIN32)
    HANDLE timer = nullptr;
#endif
};

//==============================================================================
struct Dist
{
    bool valid = false;
    double minV = 0, med = 0, p99 = 0, p999 = 0, maxV = 0, mean = 0;
};

Dist computeDist (std::vector<double> v)
{
    Dist d;
    if (v.empty())
        return d;
    std::sort (v.begin(), v.end());
    const size_t n = v.size();
    auto at = [&] (double q) { return v[std::min (n - 1, (size_t) std::llround (q * (double) (n - 1)))]; };
    d.valid = true;
    d.minV = v.front();
    d.maxV = v.back();
    d.med  = at (0.5);
    d.p99  = at (0.99);
    d.p999 = at (0.999);
    double sum = 0;
    for (double x : v) sum += x;
    d.mean = sum / (double) n;
    return d;
}

//==============================================================================
struct Spike { int block; double tRelMs; double durMs; };

struct RunResult
{
    int depth = 0;
    int blocks = 0;
    uint32_t underruns = 0, underrunsPostWarmup = 0;
    std::vector<int> underrunBlocks;         // first few indices
    bool pumpFailed = false;
    int failBlock = -1;
    std::string failError;
    Dist backendMs, jitterMs;
    std::vector<Spike> spikes;
    double budgetMs = 0;
};

//==============================================================================
// One measured run at a fixed pipeline depth. Creates a fresh backend
// (prepare bakes pipelineLatencyMs into the delay compensation, so depth
// cannot change live), wraps it in the instrumented decorator, runs the real
// GpuAsyncPipelineT, and drives push/pop at the metronomic block cadence.
//==============================================================================
RunResult runOneDepth (const Config& cfg, const std::string& deviceId, int depth,
                       const std::vector<std::vector<float>>& inputRing)
{
    RunResult r;
    r.depth = depth;
    r.budgetMs = cfg.blockMs();

    const double latencyMs = depth * cfg.blockMs();

    std::unique_ptr<spatcore::gpu::IWfsBackend> wfs;
    std::unique_ptr<spatcore::gpu::IObBackend> ob;
    spatcore::gpu::IGpuBackend* base = nullptr;

    // IWfsBackend and IObBackend expose the identical prepare/matrix surface
    // but are unrelated types (same situation as offline-render's
    // renderGpuCommon); the small lambda dance below keeps one code path.
    scenario::WfsMatrices m;
    m.allocate (cfg.numIn, cfg.numOut);
    scenario::applyWfsTick (cfg.scenarioId, 0, cfg.numIn, cfg.numOut, m);
    const auto fr = scenario::frSettings (cfg.scenarioId);

    auto setupCommon = [&] (auto& backend) -> bool
    {
        if (! backend->prepare (cfg.numIn, cfg.numOut, cfg.block, cfg.sr,
                                latencyMs, /*maxDelaySeconds*/ 1.0))
        {
            std::fprintf (stderr, "FATAL: prepare() failed (depth %d): %s\n",
                          depth, backend->getLastError().c_str());
            return false;
        }
        backend->setMatrixPointers (m.delayMs.data(), m.levels.data(), m.hfDb.data(),
                                    m.frDelayMs.data(), m.frLevels.data(), m.frHfDb.data());
        for (int in = 0; in < cfg.numIn; ++in)
        {
            backend->setFRFilterParams (in, fr.lowCutActive, fr.lowCutFreq,
                                        fr.highShelfActive, fr.highShelfFreq,
                                        fr.highShelfGain, fr.highShelfSlope);
            backend->setFRDiffusion (in, fr.diffusionPercent);
        }
        return true;
    };

    if (! cfg.scatter)
    {
        wfs = spatcore::gpu::makeWfsBackend (deviceId);
        if (wfs == nullptr || ! setupCommon (wfs))
            { r.pumpFailed = true; r.failError = "backend create/prepare failed"; return r; }
        base = wfs.get();
    }
    else
    {
        ob = spatcore::gpu::makeObBackend (deviceId);
        if (ob == nullptr || ! setupCommon (ob))
            { r.pumpFailed = true; r.failError = "backend create/prepare failed"; return r; }
        base = ob.get();
    }

    if (depth == cfg.depths.front())
        std::fprintf (stderr, "note: device: %s\n", base->getDeviceName().c_str());

    const int totalBlocks = cfg.totalBlocks();

    InstrumentedBackend instr;
    instr.begin (base, (size_t) totalBlocks + 64);

    spatcore::gpu::GpuAsyncPipelineT<InstrumentedBackend> pipeline;
    if (! pipeline.prepare (&instr, cfg.numIn, cfg.numOut, cfg.block, cfg.sr, depth))
    {
        r.pumpFailed = true;
        r.failError = "pipeline prepare failed: " + pipeline.getLastError().toStdString();
        base->release();
        return r;
    }

    juce::AudioBuffer<float> inBlock (cfg.numIn, cfg.block);
    juce::AudioBuffer<float> outBlock (cfg.numOut, cfg.block);

    std::vector<double> jitter;
    jitter.reserve ((size_t) totalBlocks);

    const int srInt = (int) cfg.sr;
    const int ringLen = (int) inputRing.front().size();
    const int spikeCap = 10000;

    Metronome metro;

#if defined(_WIN32)
    ::timeBeginPeriod (1);
#endif
    // Same elevation the app's audio/pump threads get (MMCSS "Pro Audio" via
    // runtime-loaded avrt.dll; falls back to THREAD_PRIORITY_HIGHEST).
    spatcore::rt::setCurrentThreadAudioPriority (cfg.blockMs(), cfg.blockMs() * 0.5);

    const double t0 = nowMs() + 5.0;   // small lead-in before the first deadline
    uint32_t lastUnderruns = 0;
    int lastTick = 0;

    for (int b = 0; b < totalBlocks; ++b)
    {
        const double deadline = t0 + (double) b * cfg.blockMs();
        const double over = metro.waitUntil (deadline);
        jitter.push_back (over);

        const int64_t startSample = (int64_t) b * cfg.block;

        // 50 Hz matrix timeline, same cadence as the app's timer thread. The
        // pump thread snapshots these arrays concurrently — the app's own
        // benign torn-float race, accepted by design.
        const int tick = (int) ((startSample * 50) / srInt);
        if (tick != lastTick)
        {
            scenario::applyWfsTick (cfg.scenarioId, tick, cfg.numIn, cfg.numOut, m);
            lastTick = tick;
        }

        // Input from the precomputed circular per-channel signal.
        for (int ch = 0; ch < cfg.numIn; ++ch)
        {
            float* dst = inBlock.getWritePointer (ch);
            const float* src = inputRing[(size_t) ch].data();
            const int pos = (int) (startSample % ringLen);
            const int first = std::min (cfg.block, ringLen - pos);
            std::memcpy (dst, src + pos, (size_t) first * sizeof (float));
            if (first < cfg.block)
                std::memcpy (dst + first, src, (size_t) (cfg.block - first) * sizeof (float));
        }

        pipeline.pushInput (inBlock, cfg.numIn, 0, cfg.block);
        pipeline.popOutput (outBlock, cfg.numOut, 0, cfg.block);

        const uint32_t u = pipeline.getUnderrunCount();
        if (u != lastUnderruns)
        {
            for (uint32_t k = lastUnderruns; k < u && r.underrunBlocks.size() < 50; ++k)
                r.underrunBlocks.push_back (b);
            if (b >= cfg.warmup)
                r.underrunsPostWarmup += u - lastUnderruns;
            lastUnderruns = u;
        }

        // The pump exits permanently on one backend failure and popOutput then
        // silence-fills without counting underruns — a failed run must never
        // read as clean.
        if (pipeline.hasPumpFailed())
        {
            r.pumpFailed = true;
            r.failBlock = b;
            r.failError = base->getLastError();
            break;
        }

        r.blocks = b + 1;
    }

#if defined(_WIN32)
    ::timeEndPeriod (1);
#endif

    r.underruns = pipeline.getUnderrunCount();
    pipeline.release();
    base->release();

    // Distributions exclude the warmup blocks; the spike log covers everything
    // (a first-blocks stall is still real evidence, just labeled by index).
    const size_t recorded = std::min (instr.count.load (std::memory_order_acquire),
                                      instr.samples.size());
    std::vector<double> durs;
    durs.reserve (recorded);
    for (size_t i = 0; i < recorded; ++i)
    {
        if ((int) i >= cfg.warmup)
            durs.push_back (instr.samples[i].durMs);
        if (instr.samples[i].durMs >= cfg.spikeMs && (int) r.spikes.size() < spikeCap)
            r.spikes.push_back ({ (int) i, instr.samples[i].tRelMs, instr.samples[i].durMs });
    }
    r.backendMs = computeDist (std::move (durs));

    if ((int) jitter.size() > cfg.warmup)
        r.jitterMs = computeDist (std::vector<double> (jitter.begin() + cfg.warmup, jitter.end()));

    return r;
}

//==============================================================================
void printResult (const Config& cfg, const RunResult& r)
{
    std::printf ("depth %d: blocks=%d underruns=%u (post-warmup %u)%s\n",
                 r.depth, r.blocks, r.underruns, r.underrunsPostWarmup,
                 r.pumpFailed ? "  ** PUMP FAILED **" : "");
    if (r.pumpFailed)
        std::printf ("  FAILED at block %d: %s\n", r.failBlock, r.failError.c_str());
    if (r.backendMs.valid)
        std::printf ("  backendMs  min=%.3f med=%.3f p99=%.3f p999=%.3f max=%.3f mean=%.3f  budget=%.3f\n",
                     r.backendMs.minV, r.backendMs.med, r.backendMs.p99,
                     r.backendMs.p999, r.backendMs.maxV, r.backendMs.mean, r.budgetMs);
    if (r.jitterMs.valid)
        std::printf ("  wakeJitter med=%.3f p99=%.3f max=%.3f\n",
                     r.jitterMs.med, r.jitterMs.p99, r.jitterMs.maxV);
    if (! r.spikes.empty())
    {
        std::printf ("  spikes(>=%.2fms): %d  [", cfg.spikeMs, (int) r.spikes.size());
        const int show = std::min ((int) r.spikes.size(), 8);
        for (int i = 0; i < show; ++i)
            std::printf ("%s#%d@%.1fs:%.2fms", i > 0 ? " " : "",
                         r.spikes[(size_t) i].block,
                         r.spikes[(size_t) i].tRelMs / 1000.0,
                         r.spikes[(size_t) i].durMs);
        std::printf ("%s]\n", (int) r.spikes.size() > show ? " ..." : "");
    }
    if (! r.underrunBlocks.empty())
    {
        std::printf ("  underrun blocks: [");
        const int show = std::min ((int) r.underrunBlocks.size(), 12);
        for (int i = 0; i < show; ++i)
            std::printf ("%s%d", i > 0 ? " " : "", r.underrunBlocks[(size_t) i]);
        std::printf ("%s]\n", (int) r.underrunBlocks.size() > show ? " ..." : "");
    }
    std::fflush (stdout);
}

void appendDistJson (juce::String& s, const char* name, const Dist& d)
{
    if (! d.valid)
        return;
    s << ", \"" << name << "\": { \"min\": " << juce::String (d.minV, 5)
      << ", \"median\": " << juce::String (d.med, 5)
      << ", \"p99\": " << juce::String (d.p99, 5)
      << ", \"p999\": " << juce::String (d.p999, 5)
      << ", \"max\": " << juce::String (d.maxV, 5)
      << ", \"mean\": " << juce::String (d.mean, 5) << " }";
}

bool writeJson (const juce::File& f, const Config& cfg, const std::string& deviceId,
                const std::vector<RunResult>& runs)
{
    juce::String s;
    s << "{\n"
      << "  \"device\": \"" << juce::String (deviceId) << "\",\n"
      << "  \"path\": \"" << (cfg.scatter ? "scatter" : "gather") << "\",\n"
      << "  \"scenario\": \"" << scenario::name (cfg.scenarioId) << "\",\n"
      << "  \"sr\": " << cfg.sr << ", \"block\": " << cfg.block
      << ", \"in\": " << cfg.numIn << ", \"out\": " << cfg.numOut
      << ", \"seconds\": " << cfg.seconds
      << ", \"warmup\": " << cfg.warmup
      << ", \"budgetMs\": " << juce::String (cfg.blockMs(), 5) << ",\n"
      << "  \"runs\": [\n";

    for (size_t i = 0; i < runs.size(); ++i)
    {
        const RunResult& r = runs[i];
        s << "    { \"depth\": " << r.depth
          << ", \"blocks\": " << r.blocks
          << ", \"underruns\": " << (int64_t) r.underruns
          << ", \"underrunsPostWarmup\": " << (int64_t) r.underrunsPostWarmup
          << ", \"pumpFailed\": " << (r.pumpFailed ? "true" : "false");
        if (r.pumpFailed)
            s << ", \"failBlock\": " << r.failBlock
              << ", \"failError\": \"" << juce::String (r.failError).replace ("\"", "'") << "\"";
        appendDistJson (s, "backendMs", r.backendMs);
        appendDistJson (s, "wakeJitterMs", r.jitterMs);
        s << ", \"spikes\": [";
        for (size_t k = 0; k < r.spikes.size(); ++k)
            s << (k > 0 ? ", " : "")
              << "{ \"block\": " << r.spikes[k].block
              << ", \"tMs\": " << juce::String (r.spikes[k].tRelMs, 2)
              << ", \"ms\": " << juce::String (r.spikes[k].durMs, 4) << " }";
        s << "] }" << (i + 1 < runs.size() ? "," : "") << "\n";
    }
    s << "  ]\n}\n";

    f.getParentDirectory().createDirectory();
    return f.replaceWithText (s);
}

//==============================================================================
// Plugin dir + device resolution — same contract as offline-render.
//==============================================================================
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

std::string resolveGpuDeviceId (const std::string& requested, std::string& whyNot)
{
    auto& mgr = spatcore::gpu::GpuDeviceManager::instance();

    if (! requested.empty())
    {
        const auto* d = mgr.find (requested);
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

void usage()
{
    std::fprintf (stderr,
        "usage: pipeline-bench [--device cuda:0] [--plugin-dir <dir with wfs_cuda.dll>]\n"
        "                      [--sr 96000] [--block 64] [--in 64] [--out 128]\n"
        "                      [--path gather|scatter] [--scenario static|moving|fr-toggle]\n"
        "                      [--seconds 30] [--depth N | --depth-sweep A..B]\n"
        "                      [--spike-ms 1.0] [--warmup 32] [--json out.json]\n"
        "                      [--list-devices]\n"
        "\n"
        "Drives the real GpuAsyncPipelineT (pump thread + rings + depth cushion)\n"
        "from a metronomic simulated audio callback and reports per-depth underruns,\n"
        "backend-time distributions, callback wake jitter, and a timestamped spike\n"
        "log. Default shape: 96 kHz / 64 samples, 64 in x 128 out, scenario 'moving'.\n"
        "\n"
        "exit codes: 0 ok, 2 usage, 6 GPU/plugin unavailable\n");
}

} // namespace

//==============================================================================
int main (int argc, char* argv[])
{
    Config cfg;
    bool listDevices = false;
    bool depthSet = false;

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

        if      (a == "--device")     cfg.deviceArg = next();
        else if (a == "--plugin-dir") cfg.pluginDirArg = next();
        else if (a == "--sr")         cfg.sr = std::atof (next().c_str());
        else if (a == "--block")      cfg.block = std::atoi (next().c_str());
        else if (a == "--in")         cfg.numIn = std::atoi (next().c_str());
        else if (a == "--out")        cfg.numOut = std::atoi (next().c_str());
        else if (a == "--seconds")    cfg.seconds = std::atof (next().c_str());
        else if (a == "--warmup")     cfg.warmup = std::atoi (next().c_str());
        else if (a == "--spike-ms")   cfg.spikeMs = std::atof (next().c_str());
        else if (a == "--json")       cfg.jsonArg = next();
        else if (a == "--list-devices") listDevices = true;
        else if (a == "--path")
        {
            const std::string p = next();
            if (p == "gather")       cfg.scatter = false;
            else if (p == "scatter") cfg.scatter = true;
            else { std::fprintf (stderr, "error: unknown path '%s'\n", p.c_str()); return 2; }
        }
        else if (a == "--scenario")
        {
            if (! scenario::fromName (next(), cfg.scenarioId))
                { std::fprintf (stderr, "error: unknown scenario\n"); return 2; }
        }
        else if (a == "--depth")
        {
            cfg.depths = { std::atoi (next().c_str()) };
            depthSet = true;
        }
        else if (a == "--depth-sweep")
        {
            const std::string v = next();
            const auto dots = v.find ("..");
            if (dots == std::string::npos)
                { std::fprintf (stderr, "error: --depth-sweep wants A..B\n"); return 2; }
            const int lo = std::atoi (v.substr (0, dots).c_str());
            const int hi = std::atoi (v.substr (dots + 2).c_str());
            if (lo < 1 || hi < lo || hi > 16)
                { std::fprintf (stderr, "error: bad sweep range %d..%d\n", lo, hi); return 2; }
            cfg.depths.clear();
            for (int d = lo; d <= hi; ++d)
                cfg.depths.push_back (d);
            depthSet = true;
        }
        else if (a == "--help" || a == "-h") { usage(); return 0; }
        else
        {
            std::fprintf (stderr, "error: unknown argument '%s'\n", a.c_str());
            usage();
            return 2;
        }
    }

    if (cfg.sr <= 0 || cfg.block <= 0 || cfg.numIn <= 0 || cfg.numOut <= 0
        || cfg.seconds <= 0 || cfg.warmup < 0)
    {
        std::fprintf (stderr, "error: invalid size/rate arguments\n");
        return 2;
    }
    if (! depthSet)
        cfg.depths = { 1, 2, 3, 4, 5, 6, 7, 8 };

    // Plugin dir first: enumeration only needs the driver runtime, but backend
    // creation dlopens the vendor plugin.
    juce::File pluginDir;
    if (! cfg.pluginDirArg.empty())
        pluginDir = juce::File::getCurrentWorkingDirectory()
                        .getChildFile (juce::String (cfg.pluginDirArg));
    else
        pluginDir = autoProbePluginDir();

    if (pluginDir.isDirectory())
    {
        addPluginDirToSearchPath (pluginDir);
        std::fprintf (stderr, "note: GPU plugin dir: %s\n",
                      pluginDir.getFullPathName().toRawUTF8());
    }

    if (listDevices)
    {
        for (const auto& d : spatcore::gpu::GpuDeviceManager::instance().devices())
            std::printf ("%s  %s\n", d.id.c_str(), d.name.c_str());
        return 0;
    }

    std::string whyNot;
    const std::string deviceId = resolveGpuDeviceId (cfg.deviceArg, whyNot);
    if (deviceId.empty())
    {
        std::fprintf (stderr, "error: GPU unavailable: %s\n", whyNot.c_str());
        return 6;
    }

    std::fprintf (stderr, "pipeline-bench: device=%s path=%s scenario=%s sr=%g block=%d "
                          "in=%d out=%d seconds=%g budgetMs=%.4f\n",
                  deviceId.c_str(), cfg.scatter ? "scatter" : "gather",
                  scenario::name (cfg.scenarioId), cfg.sr, cfg.block,
                  cfg.numIn, cfg.numOut, cfg.seconds, cfg.blockMs());

    // Precompute a circular 2 s input signal per channel (the scenario input is
    // a pure function of sample index; generating 64 ch x 64 samples of sines
    // inside the simulated callback would add avoidable callback load).
    const int ringLen = (int) (2.0 * cfg.sr);
    std::vector<std::vector<float>> inputRing ((size_t) cfg.numIn,
                                               std::vector<float> ((size_t) ringLen));
    for (int ch = 0; ch < cfg.numIn; ++ch)
        for (int s = 0; s < ringLen; ++s)
            inputRing[(size_t) ch][(size_t) s] =
                scenario::inputSample (cfg.scenarioId, ch, s, cfg.sr);

    std::vector<RunResult> runs;
    for (int depth : cfg.depths)
    {
        RunResult r = runOneDepth (cfg, deviceId, depth, inputRing);
        printResult (cfg, r);
        runs.push_back (std::move (r));
    }

    if (! cfg.jsonArg.empty())
    {
        const auto f = juce::File::getCurrentWorkingDirectory()
                           .getChildFile (juce::String (cfg.jsonArg));
        if (writeJson (f, cfg, deviceId, runs))
            std::fprintf (stderr, "note: JSON written to %s\n",
                          f.getFullPathName().toRawUTF8());
        else
            std::fprintf (stderr, "warning: could not write %s\n",
                          f.getFullPathName().toRawUTF8());
    }

    return 0;
}
