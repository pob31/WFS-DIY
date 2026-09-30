//==============================================================================
// Reverb audition - listening material and a measured sheet for the effects
// reverb's presets (effects plan §12.10). NOT a gate: nothing here is hashed
// or baselined, and nothing in the render paths calls it.
//
//   offline-render --audition <out-dir> [--audition-input <file.wav>] [--sr 48000]
//
// writes into <out-dir>:
//
//   reel-<source>.wav / .txt  every preset in id order on one source: the dry
//                             (peaking at -18 dBFS) plus the module's wet (Mix
//                             100 %), the preset changing at the start of each
//                             segment, so the change itself - the old tail
//                             spilling over under the new one - is heard as
//                             well. The .txt gives the time each preset starts.
//                             32-bit float, and the levels are the module's.
//   presets.csv               per preset, from one pink-noise burst: the decay
//                             measured at 125 Hz and 1 kHz (T20 of an octave
//                             band) against the nominal, the wet energy against
//                             the dry and against Medium Hall's, the wet peak,
//                             and how alike eight returns of the same burst are
//                             (mean |r| between noise keys 1..8).
//
// The sources are synthetic and deterministic - pink bursts, a snare, a
// plucked arpeggio, a sung vowel, a sweep; --audition-input adds a reel of a
// real recording (mixed to mono, the first 12 s, rendered at its own rate).
//==============================================================================

#include <JuceHeader.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>
#include "../../../spatcore/effects/EffectChain.h"      // ModuleSlot, createModule, kSlots
#include "../../../spatcore/effects/EffectPresets.h"

int runReverbAudition (const std::string& outDir, const std::string& inputWav, double sr);

namespace
{
using namespace spatcore::effects;
using Signal = std::vector<float>;

constexpr int kReverbSlot = 8;
constexpr int kBlock = 256;
constexpr double kPi = 3.14159265358979323846;

//==============================================================================
// Sources

struct Rng
{
    std::uint32_t s;
    float next() noexcept
    {
        s = s * 1664525u + 1013904223u;
        return static_cast<float> (s >> 8) * (2.0f / 16777216.0f) - 1.0f;
    }
};

// Paul Kellet's refined pink filter over white noise.
struct Pink
{
    float b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
    float next (float w) noexcept
    {
        b0 = 0.99886f * b0 + w * 0.0555179f;
        b1 = 0.99332f * b1 + w * 0.0750759f;
        b2 = 0.96900f * b2 + w * 0.1538520f;
        b3 = 0.86650f * b3 + w * 0.3104856f;
        b4 = 0.55000f * b4 + w * 0.5329522f;
        b5 = -0.7616f * b5 - w * 0.0168980f;
        const float out = b0 + b1 + b2 + b3 + b4 + b5 + b6 + w * 0.5362f;
        b6 = w * 0.115926f;
        return out;
    }
};

int samples (double seconds, double sr) { return static_cast<int> (std::lround (seconds * sr)); }

void normalisePeak (Signal& x, double peakDb)
{
    float peak = 0.0f;
    for (float v : x)
        peak = std::max (peak, std::abs (v));
    if (peak <= 0.0f)
        return;
    const float g = static_cast<float> (std::pow (10.0, peakDb / 20.0)) / peak;
    for (float& v : x)
        v *= g;
}

// A burst with 5 ms raised-cosine edges.
Signal pinkBursts (double sr, int bursts, double burstSec, double periodSec)
{
    const int len = samples (burstSec, sr), edge = samples (0.005, sr);
    Signal x (static_cast<size_t> (samples (periodSec * (bursts - 1), sr) + len), 0.0f);
    Rng rng { 12345u };
    Pink pink;

    for (int b = 0; b < bursts; ++b)
    {
        const int start = samples (periodSec * b, sr);
        for (int i = 0; i < len && start + i < static_cast<int> (x.size()); ++i)
        {
            double w = 1.0;
            if (i < edge)            w = 0.5 - 0.5 * std::cos (kPi * i / edge);
            else if (i >= len - edge) w = 0.5 - 0.5 * std::cos (kPi * (len - 1 - i) / edge);
            x[static_cast<size_t> (start + i)] = static_cast<float> (w * pink.next (rng.next()));
        }
    }
    normalisePeak (x, -12.0);
    return x;
}

// Four hits: band-limited noise on a falling 185 Hz body.
Signal snare (double sr)
{
    const double period = 0.5;
    const int hitLen = samples (0.6, sr);
    Signal x (static_cast<size_t> (samples (period * 3, sr) + hitLen), 0.0f);
    Rng rng { 777u };
    const double lpA = std::exp (-2.0 * kPi * 7000.0 / sr);

    for (int h = 0; h < 4; ++h)
    {
        const int start = samples (period * h, sr);
        double lp = 0.0, prev = 0.0, phase = 0.0;
        for (int i = 0; i < hitLen && start + i < static_cast<int> (x.size()); ++i)
        {
            const double t = i / sr;
            const double white = rng.next();
            lp = lpA * lp + (1.0 - lpA) * white;
            const double hp = lp - prev;                     // a gentle high pass
            prev = lp;
            const double noise = 3.0 * hp * std::exp (-t / 0.07);
            phase += 2.0 * kPi * (185.0 * (1.0 + 0.4 * std::exp (-t / 0.02))) / sr;
            const double body = 0.6 * std::sin (phase) * std::exp (-t / 0.09);
            x[static_cast<size_t> (start + i)] += static_cast<float> (noise + body);
        }
    }
    normalisePeak (x, -12.0);
    return x;
}

// Karplus-Strong: C4 E4 G4 C5, the last one left to ring.
Signal pluck (double sr)
{
    const double notes[] = { 261.63, 329.63, 392.00, 523.25 };
    const double step = 0.45;
    Signal x (static_cast<size_t> (samples (step * 3 + 1.6, sr)), 0.0f);
    Rng rng { 4242u };

    for (int n = 0; n < 4; ++n)
    {
        const int period = std::max (2, static_cast<int> (std::lround (sr / notes[n])));
        std::vector<double> ring (static_cast<size_t> (period));
        for (auto& v : ring)
            v = rng.next();

        const int start = samples (step * n, sr);
        const int len = samples (n == 3 ? 1.6 : step + 0.6, sr);
        size_t k = 0;
        for (int i = 0; i < len && start + i < static_cast<int> (x.size()); ++i)
        {
            const size_t k1 = (k + 1) % ring.size();
            const double y = 0.4985 * (ring[k] + ring[k1]);
            x[static_cast<size_t> (start + i)] += static_cast<float> (ring[k]);
            ring[k] = y;
            k = k1;
        }
    }
    normalisePeak (x, -12.0);
    return x;
}

// A sung "ah" at G3: additive harmonics under three formants, with vibrato.
Signal vowel (double sr)
{
    const double f0 = 196.0, dur = 1.6;
    const struct { double f, bw, g; } formants[] = { { 730.0, 80.0, 1.0 }, { 1090.0, 90.0, 0.5 }, { 2440.0, 120.0, 0.25 } };
    const int numHarm = std::max (1, static_cast<int> (std::min (8000.0, 0.45 * sr) / f0));

    std::vector<double> amp (static_cast<size_t> (numHarm + 1), 0.0), phase (static_cast<size_t> (numHarm + 1), 0.0);
    for (int k = 1; k <= numHarm; ++k)
    {
        double a = 0.0;
        for (const auto& fm : formants)
        {
            const double d = (k * f0 - fm.f) / fm.bw;
            a += fm.g / (1.0 + d * d);
        }
        amp[static_cast<size_t> (k)] = (a + 0.02) / k;       // a 6 dB/octave source under the formants
    }

    Signal x (static_cast<size_t> (samples (dur, sr)), 0.0f);
    for (int i = 0; i < static_cast<int> (x.size()); ++i)
    {
        const double t = i / sr;
        const double env = std::min ({ 1.0, t / 0.06, (dur - t) / 0.15 });
        const double f = f0 * (1.0 + 0.012 * std::sin (2.0 * kPi * 5.5 * t) * std::min (1.0, t / 0.4));
        double v = 0.0;
        for (int k = 1; k <= numHarm; ++k)
        {
            phase[static_cast<size_t> (k)] += 2.0 * kPi * k * f / sr;
            v += amp[static_cast<size_t> (k)] * std::sin (phase[static_cast<size_t> (k)]);
        }
        x[static_cast<size_t> (i)] = static_cast<float> (env * v);
    }
    normalisePeak (x, -12.0);
    return x;
}

// A 2 s logarithmic sweep, 60 Hz to 12 kHz.
Signal sweep (double sr)
{
    const double f1 = 60.0, f2 = std::min (12000.0, 0.45 * sr), dur = 2.0;
    const double k = std::log (f2 / f1);
    Signal x (static_cast<size_t> (samples (dur, sr)), 0.0f);
    for (int i = 0; i < static_cast<int> (x.size()); ++i)
    {
        const double t = i / sr;
        const double ph = 2.0 * kPi * f1 * dur / k * (std::exp (t / dur * k) - 1.0);
        const double env = std::min ({ 1.0, t / 0.02, (dur - t) / 0.02 });
        x[static_cast<size_t> (i)] = static_cast<float> (env * std::sin (ph));
    }
    normalisePeak (x, -12.0);
    return x;
}

bool loadMono (const juce::File& f, Signal& out, double& rate)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (f));
    if (reader == nullptr)
        return false;

    rate = reader->sampleRate;
    const int n = static_cast<int> (std::min<juce::int64> (reader->lengthInSamples, static_cast<juce::int64> (12.0 * rate)));
    juce::AudioBuffer<float> buf (std::min (2, static_cast<int> (reader->numChannels)), n);   // read() fills two at most
    reader->read (&buf, 0, n, 0, true, true);

    out.assign (static_cast<size_t> (n), 0.0f);
    for (int c = 0; c < buf.getNumChannels(); ++c)
        for (int i = 0; i < n; ++i)
            out[static_cast<size_t> (i)] += buf.getSample (c, i) / static_cast<float> (buf.getNumChannels());
    return n > 0;
}

//==============================================================================
// Rendering

struct Wet
{
    Signal wet;
    std::uint32_t nanTrips = 0;
};

// The reverb module alone in a slot, fully wet; each (sample, preset) pair
// selects a preset at the first block boundary at or after that sample, as
// the app's parameter publisher would. The first one lands on a fresh module,
// so it applies at once; every later one spills over.
Wet renderWet (const Signal& dry, double sr, const std::vector<std::pair<int, int>>& presetAt, std::uint32_t noiseKey)
{
    ChainConfig cfg;
    cfg.sampleRate = sr;
    cfg.maxBlock = kBlock;
    cfg.noiseKey = noiseKey;

    const auto type = kSlots[kReverbSlot].type;
    const int instance = static_cast<int> (kSlots[kReverbSlot].instance);

    ModuleSlot slot;
    slot.prepare (cfg, createModule (type, instance, cfg));

    EffectChannelParams params;
    params.reverb.bypass = 0;
    params.reverb.mix = 100.0f;

    Wet out;
    out.wet.assign (dry.size(), 0.0f);
    std::vector<float> buf (static_cast<size_t> (kBlock));
    size_t next = 0;

    for (int start = 0; start < static_cast<int> (dry.size()); start += kBlock)
    {
        bool changed = false;
        while (next < presetAt.size() && presetAt[next].first <= start)
        {
            applyReverbPreset (params.reverb, presetAt[next].second);
            ++next;
            changed = true;
        }
        if (changed || start == 0)
        {
            ++params.revision;
            slot.applyParams (params, instance);
        }

        const int len = std::min (kBlock, static_cast<int> (dry.size()) - start);
        std::copy (dry.begin() + start, dry.begin() + start + len, buf.begin());
        slot.process (buf.data(), len);
        std::copy (buf.begin(), buf.begin() + len, out.wet.begin() + start);
    }

    out.nanTrips = slot.nanTrips.load();
    return out;
}

//==============================================================================
// Measurement

struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    static Biquad bandPass (double f, double q, double sr)
    {
        const double w = 2.0 * kPi * f / sr, alpha = std::sin (w) / (2.0 * q), a0 = 1.0 + alpha;
        Biquad bq;
        bq.b0 = alpha / a0;
        bq.b1 = 0.0;
        bq.b2 = -alpha / a0;
        bq.a1 = -2.0 * std::cos (w) / a0;
        bq.a2 = (1.0 - alpha) / a0;
        return bq;
    }

    double run (double x) noexcept
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};

// T20 of an octave band around f, extrapolated to 60 dB: the backward-
// integrated energy of the band-passed signal from `from` on, a least-squares
// line through its -5..-25 dB stretch. -1 when the stretch is not reached.
double bandRt60 (const Signal& x, double sr, double f, int from)
{
    Biquad s1 = Biquad::bandPass (f, 1.41, sr), s2 = Biquad::bandPass (f, 1.41, sr);
    std::vector<double> e (x.size(), 0.0);
    for (size_t i = 0; i < x.size(); ++i)
    {
        const double y = s2.run (s1.run (x[i]));
        e[i] = y * y;
    }

    std::vector<double> edc (x.size() + 1, 0.0);
    for (size_t i = x.size(); i-- > 0;)
        edc[i] = edc[i + 1] + e[i];

    const double e0 = edc[static_cast<size_t> (from)];
    if (e0 <= 0.0)
        return -1.0;

    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    int n = 0;
    bool reached = false;
    for (size_t i = static_cast<size_t> (from); i < x.size(); ++i)
    {
        const double db = 10.0 * std::log10 (std::max (edc[i], 1.0e-300) / e0);
        if (db > -5.0)
            continue;
        if (db < -25.0)
        {
            reached = true;
            break;
        }
        const double t = static_cast<double> (i) / sr;
        sx += t; sy += db; sxx += t * t; sxy += t * db; ++n;
    }

    if (! reached || n < 16)
        return -1.0;
    const double slope = (n * sxy - sx * sy) / (n * sxx - sx * sx);
    return slope < 0.0 ? -60.0 / slope : -1.0;
}

double energy (const Signal& x, size_t from = 0, size_t to = SIZE_MAX)
{
    double s = 0.0;
    for (size_t i = from; i < std::min (to, x.size()); ++i)
        s += static_cast<double> (x[i]) * x[i];
    return s;
}

double db (double ratio) { return ratio > 0.0 ? 10.0 * std::log10 (ratio) : -999.0; }

double meanAbsCorrelation (const std::vector<Signal>& wets, size_t from, size_t to)
{
    double sum = 0.0;
    int pairs = 0;
    for (size_t a = 0; a < wets.size(); ++a)
        for (size_t b = a + 1; b < wets.size(); ++b)
        {
            double sab = 0, saa = 0, sbb = 0;
            for (size_t i = from; i < std::min (to, wets[a].size()); ++i)
            {
                sab += static_cast<double> (wets[a][i]) * wets[b][i];
                saa += static_cast<double> (wets[a][i]) * wets[a][i];
                sbb += static_cast<double> (wets[b][i]) * wets[b][i];
            }
            if (saa > 0.0 && sbb > 0.0)
            {
                sum += std::abs (sab) / std::sqrt (saa * sbb);
                ++pairs;
            }
        }
    return pairs > 0 ? sum / pairs : 0.0;
}

//==============================================================================
// Output

bool writeWav32 (const juce::File& f, const Signal& x, double sr)
{
    f.deleteFile();
    auto os = std::make_unique<juce::FileOutputStream> (f);
    if (! os->openedOk())
        return false;

    juce::WavAudioFormat fmt;
    std::unique_ptr<juce::AudioFormatWriter> writer (fmt.createWriterFor (os.get(), sr, 1, 32, {}, 0));
    if (writer == nullptr)
        return false;
    os.release();

    const float* ch[] = { x.data() };
    return writer->writeFromFloatArrays (ch, 1, static_cast<int> (x.size()));
}

std::vector<int> presetIds()
{
    std::vector<int> ids;
    for (int t = 0; t < static_cast<int> (ReverbType::Count); ++t)
        if (findReverbPreset (t) != nullptr)
            ids.push_back (t);
    return ids;
}

const char* modelName (int m)
{
    switch (resolveReverbModel (m))
    {
        case static_cast<int> (ReverbModel::Plate):         return "Plate";
        case static_cast<int> (ReverbModel::ModulatedHall): return "Modulated Hall";
        case static_cast<int> (ReverbModel::Shimmer):       return "Shimmer";
        default:                                            return "FDN";
    }
}

const char* erName (int p)
{
    static const char* names[] = { "Off", "Room", "Chamber", "Hall", "Cathedral" };
    return p >= 0 && p < 5 ? names[p] : "?";
}

// One reel: every preset in id order on one source, the preset changing at
// each segment's first block. Each segment is the phrase and then silence long
// enough for the preset's slowest band to fall most of the way (clamped 2.5..9 s).
// The phrase peaks at -18 dBFS: a long reverb builds up on a held tone - the
// sweep reaches about +12 dB over its dry peak at the modes of the longest
// presets - so the reels keep that much headroom.
bool writeReel (const juce::File& dir, const std::string& name, Signal phrase, double sr, std::uint32_t& nanTrips)
{
    normalisePeak (phrase, -18.0);

    std::vector<std::pair<int, int>> presetAt;
    std::string sheet = "reel-" + name + ": each preset from the time given, dry + wet (Mix 100 %)\n\n";
    Signal dry;

    for (int id : presetIds())
    {
        const auto* row = findReverbPreset (id);
        const double tail = std::clamp (1.1 * row->rt60 * std::max (1.0f, row->rt60LowMult), 2.5, 9.0);

        int start = static_cast<int> (dry.size());
        start = (start + kBlock - 1) / kBlock * kBlock;          // a block boundary, as the app would
        dry.resize (static_cast<size_t> (start), 0.0f);
        presetAt.emplace_back (start, id);

        char line[200];
        const double t = start / sr;
        std::snprintf (line, sizeof line, "%2d:%05.2f  %2d  %-24s %-15s ER %-9s RT60 %.2f s\n",
                       static_cast<int> (t / 60.0), std::fmod (t, 60.0), id, row->name,
                       modelName (row->model), erName (row->erProfile), static_cast<double> (row->rt60));
        sheet += line;

        dry.insert (dry.end(), phrase.begin(), phrase.end());
        dry.resize (dry.size() + static_cast<size_t> (samples (tail, sr)), 0.0f);
    }

    const auto wet = renderWet (dry, sr, presetAt, 1u);
    nanTrips += wet.nanTrips;

    Signal mix (dry.size());
    for (size_t i = 0; i < dry.size(); ++i)
        mix[i] = dry[i] + wet.wet[i];

    const bool ok = writeWav32 (dir.getChildFile ("reel-" + juce::String (name) + ".wav"), mix, sr)
                 && dir.getChildFile ("reel-" + juce::String (name) + ".txt").replaceWithText (sheet);
    std::fprintf (stderr, "  reel-%s.wav  %.1f s\n", name.c_str(), dry.size() / sr);
    return ok;
}

} // namespace

//==============================================================================
int runReverbAudition (const std::string& outDir, const std::string& inputWav, double sr)
{
    const juce::File dir = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String (outDir));
    if (! dir.createDirectory())
    {
        std::fprintf (stderr, "error: cannot create %s\n", dir.getFullPathName().toRawUTF8());
        return 2;
    }

    std::fprintf (stderr, "reverb audition -> %s (%.0f Hz)\n", dir.getFullPathName().toRawUTF8(), sr);
    std::uint32_t nanTrips = 0;
    bool ok = true;

    // The sheet: one 250 ms pink burst, 14 s of room for the longest tail.
    Signal burst = pinkBursts (sr, 1, 0.25, 1.0);
    const int burstLen = static_cast<int> (burst.size());
    burst.resize (static_cast<size_t> (samples (14.0, sr)), 0.0f);
    const double dryEnergy = energy (burst);

    struct Row { int id; double wetDb, peakDb, t125, t1k, corr, corrEarly, corrLate; };
    std::vector<Row> rows;
    double mediumHallDb = 0.0;

    for (int id : presetIds())
    {
        std::vector<Signal> wets;
        for (std::uint32_t key = 1; key <= 8; ++key)
        {
            auto w = renderWet (burst, sr, { { 0, id } }, key);
            nanTrips += w.nanTrips;
            wets.push_back (std::move (w.wet));
        }

        const auto& w = wets[0];
        float peak = 0.0f;
        for (float v : w)
            peak = std::max (peak, std::abs (v));

        Row r;
        r.id = id;
        r.wetDb = db (energy (w) / dryEnergy);
        r.peakDb = 20.0 * std::log10 (std::max (1.0e-12f, peak) / std::pow (10.0, -12.0 / 20.0));
        r.t125 = bandRt60 (w, sr, 125.0, burstLen);
        r.t1k = bandRt60 (w, sr, 1000.0, burstLen);
        r.corr = meanAbsCorrelation (wets, 0, static_cast<size_t> (samples (2.0, sr)));
        r.corrEarly = meanAbsCorrelation (wets, 0, static_cast<size_t> (samples (0.3, sr)));      // under the burst
        r.corrLate = meanAbsCorrelation (wets, static_cast<size_t> (samples (0.4, sr)),
                                         static_cast<size_t> (samples (2.0, sr)));                // the tail
        if (id == static_cast<int> (ReverbType::MediumHall))
            mediumHallDb = r.wetDb;
        rows.push_back (r);
    }

    std::string csv = "id,name,model,er,rt60_nominal_s,rt60_low_nominal_s,t20_1k_s,t20_125_s,"
                      "wet_vs_dry_db,wet_vs_medium_hall_db,wet_peak_vs_dry_peak_db,"
                      "mean_abs_r_8_returns,mean_abs_r_first_300ms,mean_abs_r_400ms_to_2s\n";
    std::fprintf (stderr, "\n  %-24s %-15s %6s %6s | %6s %6s | %7s %7s %6s | %5s %5s %5s\n",
                  "preset", "model", "RT60", "low", "T@1k", "T@125", "wet dB", "vs MH", "peak", "|r|", "early", "tail");

    for (const auto& r : rows)
    {
        const auto* row = findReverbPreset (r.id);
        const double low = static_cast<double> (row->rt60) * row->rt60LowMult;
        char line[400];
        std::snprintf (line, sizeof line, "%d,%s,%s,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.3f,%.3f,%.3f\n",
                       r.id, row->name, modelName (row->model), erName (row->erProfile),
                       static_cast<double> (row->rt60), low, r.t1k, r.t125,
                       r.wetDb, r.wetDb - mediumHallDb, r.peakDb, r.corr, r.corrEarly, r.corrLate);
        csv += line;
        std::fprintf (stderr, "  %-24s %-15s %6.2f %6.2f | %6.2f %6.2f | %7.2f %+7.2f %6.2f | %5.3f %5.3f %5.3f\n",
                      row->name, modelName (row->model), static_cast<double> (row->rt60), low,
                      r.t1k, r.t125, r.wetDb, r.wetDb - mediumHallDb, r.peakDb, r.corr, r.corrEarly, r.corrLate);
    }
    ok = dir.getChildFile ("presets.csv").replaceWithText (csv) && ok;

    std::fprintf (stderr, "\n");
    ok = writeReel (dir, "pink", pinkBursts (sr, 3, 0.25, 0.8), sr, nanTrips) && ok;
    ok = writeReel (dir, "snare", snare (sr), sr, nanTrips) && ok;
    ok = writeReel (dir, "pluck", pluck (sr), sr, nanTrips) && ok;
    ok = writeReel (dir, "vowel", vowel (sr), sr, nanTrips) && ok;
    ok = writeReel (dir, "sweep", sweep (sr), sr, nanTrips) && ok;

    if (! inputWav.empty())
    {
        Signal user;
        double rate = sr;
        if (loadMono (juce::File::getCurrentWorkingDirectory().getChildFile (juce::String (inputWav)), user, rate))
            ok = writeReel (dir, "input", user, rate, nanTrips) && ok;
        else
        {
            std::fprintf (stderr, "error: cannot read %s\n", inputWav.c_str());
            ok = false;
        }
    }

    if (nanTrips != 0)
        std::fprintf (stderr, "WARNING: %u NaN trap trip(s) - a render produced a non-finite sample\n", nanTrips);

    return ok && nanTrips == 0 ? 0 : (nanTrips != 0 ? 8 : 2);
}
