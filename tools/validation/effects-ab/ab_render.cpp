//==============================================================================
// ab_render - our side of the Max/MSP A/B harness (effects plan section 10).
//
// Reads the deterministic input WAV that gen_input.py produced, pushes it
// through ONE spatcore effects module with parameters converted from the gen~
// prototype's inlet values by ab_mapping.h, and writes a WAV plus a sidecar
// JSON that ab_compare.py needs to judge the result.
//
//   ab-render --params params/delay.json [--in ab_input.wav] [--out ours.wav]
//             [--patch fx_delay] [--sr 48000] [--block 512]
//             [--sidecar <file>] [--expect-sha <hex>]
//   ab-render --print-mapping <fx_delay|all>
//   ab-render --list
//
// Headless, no audio device, no RNG that varies by platform - the same
// contract as tools/validation/offline-render, which this is modelled on.
// It is NOT a CI gate: section 10 says so, and the prototypes are explicitly
// starting points open to improvement rather than bit-for-bit targets.
//
// The module is driven STANDALONE (createModule -> prepare -> applyParams ->
// process), not through EffectChain, because a chain fades a slot in from
// bypass over a 5 ms time constant and that fade is not part of the module's
// sound. What the user is judging is the module.
//
// exit codes: 0 ok, 2 usage / bad JSON, 3 input WAV unreadable, 4 output
//             unwritable, 5 the mapping refused the parameters (a blocker),
//             6 the input fingerprint did not match --expect-sha
//==============================================================================

#include <JuceHeader.h>

#include <cmath>
#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../../../spatcore/effects/EffectChain.h"       // createModule + every module
#include "ab_mapping.h"
#include "sha256.h"

namespace
{

//==============================================================================
struct Config
{
    juce::String paramsPath;
    juce::String inPath   = "ab_input.wav";
    juce::String outPath  = "ours.wav";
    juce::String sidecarPath;
    juce::String patchOverride;
    juce::String expectSha;
    double sr = 0.0;                // 0 = take the input file's rate
    int block = 512;
};

void usage()
{
    std::fprintf (stderr,
        "usage: ab-render --params <params.json>\n"
        "                 [--in ab_input.wav] [--out ours.wav] [--sidecar <file>]\n"
        "                 [--patch fx_delay] [--sr 48000] [--block 512]\n"
        "                 [--expect-sha <hex from gen_input.py>]\n"
        "       ab-render --print-mapping <fx_delay|all>\n"
        "       ab-render --list\n"
        "\n"
        "params.json:\n"
        "  { \"patch\": \"fx_delay\",\n"
        "    \"options\": { \"dynMatchProtoTimeWiring\": false, \"shelfGainNominal\": false,\n"
        "                  \"allowUnmapped\": false },\n"
        "    \"prototype\": { \"delayTime\": 0.375, \"feedback\": 40, \"dryWet\": 50 } }\n"
        "\n"
        "Every key under \"prototype\" is a gen~ INLET NAME, spelt as the patch spells\n"
        "it, carrying the value you typed into Max. An unknown name is refused rather\n"
        "than ignored, and a missing one takes the gen~ @default (which is printed).\n"
        "The conversions to our parameter names live in ab_mapping.h and nowhere else.\n"
        "\n"
        "exit codes: 0 ok, 2 usage/JSON, 3 input unreadable, 4 output unwritable,\n"
        "            5 the mapping refused these parameters, 6 input fingerprint\n");
}

//==============================================================================
/** The one place an expected-difference block is printed.

    The adversarial review found the remedies reached the user only AFTER the
    Max render they were meant to inform: printPatch() showed severity/id/title
    and dropped `metrics` and `remedy`, while the render path printed all five.
    Both callers now go through here, so the two can never drift again.
*/
void printDiffs (const std::vector<const abmap::Diff*>& active)
{
    for (const auto* d : active)
    {
        std::printf ("    [%-5s] %-22s %s\n", d->severity, d->id, d->title);
        if (d->metrics[0] != '\0')
            std::printf ("              affects: %s\n", d->metrics);
        if (d->remedy[0] != '\0')
            std::printf ("              in Max:  %s\n", d->remedy);
        std::printf ("\n");
    }
}

//==============================================================================
void printPatch (const abmap::Patch& p)
{
    std::printf ("\n%s  ->  module '%s'  (%s)\n", p.name, p.module,
                 p.nonlinear ? "nonlinear: the -30 dB target" : "linear: the -40 dB target");
    std::printf ("  %s\n\n", p.summary);

    std::printf ("  %-4s %-20s %-10s %-9s %s\n", "in", "inlet", "default", "unit", "conversion");
    std::printf ("  %-4s %-20s %-10s %-9s %s\n", "--", "-----", "-------", "----", "----------");

    for (int i = 0; i < p.numInlets; ++i)
    {
        const abmap::Inlet& in = p.inlets[i];
        std::printf ("  %-4d %-20s %-10.6g %-9s %s\n",
                     in.index, in.name, in.defaultValue,
                     in.unit[0] != '\0' ? in.unit : "-", in.note);
    }

    // Convert the all-defaults case so the differences it declares can be shown
    // without the user having to render anything first.
    abmap::Values none;
    abmap::Options opt;
    const abmap::MapResult r = abmap::map (p, none, 48000.0, opt);

    std::printf ("\n  at the gen~ defaults, 48 kHz, the mapping sets:\n");
    for (const auto& n : r.native)
        std::printf ("    %s\n", n.c_str());

    if (! r.warnings.empty())
    {
        std::printf ("\n  and warns:\n");
        for (const auto& w : r.warnings)
            std::printf ("    ! %s\n", w.c_str());
    }

    if (! r.active.empty())
    {
        std::printf ("\n  EXPECTED DIFFERENCES at those defaults, and what to do about them\n");
        std::printf ("  (read the \"in Max\" lines BEFORE you render: they change what\n"
                     "   you type into the patch. A params.json may activate a different\n"
                     "   set - the render prints the list that actually applied.)\n\n");
        printDiffs (r.active);
    }
}

//==============================================================================
bool readWav (const juce::File& file, std::vector<float>& out, double& sampleRate, juce::String& err)
{
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatReader> reader (wav.createReaderFor (new juce::FileInputStream (file), true));

    if (reader == nullptr)
    {
        err = "cannot read " + file.getFullPathName() + " as a WAV file";
        return false;
    }

    const int n = static_cast<int> (reader->lengthInSamples);
    if (n <= 0)
    {
        err = file.getFullPathName() + " is empty";
        return false;
    }

    juce::AudioBuffer<float> buffer (static_cast<int> (reader->numChannels), n);
    if (! reader->read (&buffer, 0, n, 0, true, reader->numChannels > 1))
    {
        err = "read failed on " + file.getFullPathName();
        return false;
    }

    sampleRate = reader->sampleRate;
    out.assign (buffer.getReadPointer (0), buffer.getReadPointer (0) + n);
    return true;
}

bool writeWav (const juce::File& file, const std::vector<float>& data, double sampleRate)
{
    file.deleteFile();
    auto stream = std::make_unique<juce::FileOutputStream> (file);
    if (! stream->openedOk())
        return false;

    std::unique_ptr<juce::OutputStream> asOutput (std::move (stream));

    juce::WavAudioFormat wav;
    const auto options = juce::AudioFormatWriterOptions{}
                             .withSampleRate (sampleRate)
                             .withNumChannels (1)
                             .withBitsPerSample (32)
                             .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);

    auto writer = wav.createWriterFor (asOutput, options);
    if (writer == nullptr)
        return false;

    const float* channels[] = { data.data() };
    return writer->writeFromFloatArrays (channels, 1, static_cast<int> (data.size()));
}

/** SHA-256 of the little-endian float32 sample bytes - the same bytes
    gen_input.py hashes, so the two sides can prove they started from one file. */
std::string fingerprint (const std::vector<float>& data)
{
    return abh::Sha256::hashHex (data.data(), data.size() * sizeof (float));
}

//==============================================================================
juce::var jsonArray (const std::vector<std::string>& in)
{
    juce::Array<juce::var> a;
    for (const auto& s : in)
        a.add (juce::var (juce::String (s)));
    return juce::var (a);
}

void writeSidecar (const juce::File& file,
                   const Config& cfg,
                   const abmap::Patch& patch,
                   const abmap::MapResult& result,
                   const abmap::Options& opt,
                   const std::vector<float>& input,
                   const std::vector<float>& output,
                   double sampleRate,
                   int latency,
                   const std::string& inSha,
                   const juce::File& inFile,
                   const juce::File& outFile)
{
    auto stats = [] (const std::vector<float>& v)
    {
        double peak = 0.0, sum = 0.0;
        for (const float s : v)
        {
            const double a = std::fabs (static_cast<double> (s));
            if (a > peak) peak = a;
            sum += static_cast<double> (s) * static_cast<double> (s);
        }
        const double rms = v.empty() ? 0.0 : std::sqrt (sum / static_cast<double> (v.size()));
        return std::make_pair (peak, rms);
    };

    const auto inStats = stats (input);
    const auto outStats = stats (output);

    auto* root = new juce::DynamicObject();
    root->setProperty ("tool", "ab_render");
    root->setProperty ("patch", juce::String (patch.name));
    root->setProperty ("module", juce::String (patch.module));
    root->setProperty ("nonlinear", patch.nonlinear);
    root->setProperty ("sampleRate", sampleRate);
    root->setProperty ("block", cfg.block);
    root->setProperty ("latencySamples", latency);

    auto* opts = new juce::DynamicObject();
    opts->setProperty ("dynMatchProtoTimeWiring", opt.dynMatchProtoTimeWiring);
    opts->setProperty ("shelfGainNominal", opt.shelfGainNominal);
    opts->setProperty ("allowUnmapped", opt.allowUnmapped);
    root->setProperty ("options", juce::var (opts));

    auto* in = new juce::DynamicObject();
    in->setProperty ("path", inFile.getFullPathName());
    in->setProperty ("sha256", juce::String (inSha));
    in->setProperty ("samples", static_cast<int> (input.size()));
    in->setProperty ("peak", inStats.first);
    in->setProperty ("rms", inStats.second);
    root->setProperty ("input", juce::var (in));

    auto* out = new juce::DynamicObject();
    out->setProperty ("path", outFile.getFullPathName());
    out->setProperty ("samples", static_cast<int> (output.size()));
    out->setProperty ("peak", outStats.first);
    out->setProperty ("rms", outStats.second);
    root->setProperty ("output", juce::var (out));

    root->setProperty ("inlets", jsonArray (result.resolved));
    root->setProperty ("native", jsonArray (result.native));
    root->setProperty ("warnings", jsonArray (result.warnings));

    juce::Array<juce::var> diffs;
    for (const auto* d : result.active)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("id", juce::String (d->id));
        o->setProperty ("severity", juce::String (d->severity));
        o->setProperty ("metrics", juce::String (d->metrics));
        o->setProperty ("title", juce::String (d->title));
        o->setProperty ("why", juce::String (d->why));
        o->setProperty ("remedy", juce::String (d->remedy));
        diffs.add (juce::var (o));
    }
    root->setProperty ("expectedDifferences", juce::var (diffs));

    file.replaceWithText (juce::JSON::toString (juce::var (root), false));
}

} // namespace

//==============================================================================
int main (int argc, char* argv[])
{
    if (! abh::Sha256::selfTest())
    {
        std::fprintf (stderr, "FATAL: SHA-256 self-test failed\n");
        return 2;
    }

    Config cfg;
    juce::String printMapping;
    bool listOnly = false;

    for (int i = 1; i < argc; ++i)
    {
        const juce::String a (argv[i]);
        auto next = [&] () -> juce::String
        {
            if (i + 1 >= argc)
            {
                std::fprintf (stderr, "error: %s needs a value\n", a.toRawUTF8());
                usage();
                std::exit (2);
            }
            return juce::String (argv[++i]);
        };

        if      (a == "--params")        cfg.paramsPath = next();
        else if (a == "--in")            cfg.inPath = next();
        else if (a == "--out")           cfg.outPath = next();
        else if (a == "--sidecar")       cfg.sidecarPath = next();
        else if (a == "--patch")         cfg.patchOverride = next();
        else if (a == "--sr")            cfg.sr = next().getDoubleValue();
        else if (a == "--block")         cfg.block = next().getIntValue();
        else if (a == "--expect-sha")    cfg.expectSha = next().toLowerCase();
        else if (a == "--print-mapping") printMapping = next();
        else if (a == "--list")          listOnly = true;
        else if (a == "--help" || a == "-h") { usage(); return 0; }
        else
        {
            std::fprintf (stderr, "error: unknown argument '%s'\n", a.toRawUTF8());
            usage();
            return 2;
        }
    }

    //--------------------------------------------------------------- --list
    if (listOnly)
    {
        std::printf ("prototype patches this harness knows (Documentation/effects/):\n\n");
        std::printf ("  %-16s %-8s %-10s %s\n", "patch", "module", "class", "summary");
        for (const auto& p : abmap::kPatches)
            std::printf ("  %-16s %-8s %-10s %s\n", p.name, p.module,
                         p.nonlinear ? "nonlinear" : "linear", p.summary);
        std::printf ("\nModules with no prototype: eq1/eq2, phaser, reverb. There is nothing\n"
                     "to A/B them against - judge those against the plan, not against Max.\n");
        return 0;
    }

    //------------------------------------------------------ --print-mapping
    if (printMapping.isNotEmpty())
    {
        if (printMapping == "all")
        {
            for (const auto& p : abmap::kPatches)
                printPatch (p);
            return 0;
        }

        const auto* p = abmap::findPatch (printMapping.toStdString());
        if (p == nullptr)
        {
            std::fprintf (stderr, "error: no patch called '%s' (try --list)\n", printMapping.toRawUTF8());
            return 2;
        }
        printPatch (*p);
        return 0;
    }

    //--------------------------------------------------------------- params
    if (cfg.paramsPath.isEmpty())
    {
        std::fprintf (stderr, "error: --params is required\n");
        usage();
        return 2;
    }

    const juce::File paramsFile = juce::File::getCurrentWorkingDirectory().getChildFile (cfg.paramsPath);
    if (! paramsFile.existsAsFile())
    {
        std::fprintf (stderr, "error: %s does not exist\n", paramsFile.getFullPathName().toRawUTF8());
        return 2;
    }

    juce::var params;
    const auto parseResult = juce::JSON::parse (paramsFile.loadFileAsString(), params);
    if (parseResult.failed() || ! params.isObject())
    {
        std::fprintf (stderr, "error: %s is not a JSON object (%s)\n",
                      paramsFile.getFullPathName().toRawUTF8(),
                      parseResult.getErrorMessage().toRawUTF8());
        return 2;
    }

    const juce::String patchName = cfg.patchOverride.isNotEmpty()
                                       ? cfg.patchOverride
                                       : params.getProperty ("patch", juce::var()).toString();

    if (patchName.isEmpty())
    {
        std::fprintf (stderr, "error: no \"patch\" in %s and no --patch given\n",
                      paramsFile.getFullPathName().toRawUTF8());
        return 2;
    }

    const auto* patch = abmap::findPatch (patchName.toStdString());
    if (patch == nullptr)
    {
        std::fprintf (stderr, "error: no patch called '%s' (try --list)\n", patchName.toRawUTF8());
        return 2;
    }

    // "prototype" is the documented key; "inlets" is accepted because that is
    // what the values are.
    juce::var proto = params.getProperty ("prototype", juce::var());
    if (! proto.isObject())
        proto = params.getProperty ("inlets", juce::var());

    abmap::Values given;
    if (proto.isObject())
    {
        if (auto* obj = proto.getDynamicObject())
            for (const auto& prop : obj->getProperties())
            {
                if (! prop.value.isDouble() && ! prop.value.isInt() && ! prop.value.isInt64()
                     && ! prop.value.isBool())
                {
                    std::fprintf (stderr, "error: inlet '%s' must be a number, not '%s'\n",
                                  prop.name.toString().toRawUTF8(),
                                  prop.value.toString().toRawUTF8());
                    return 2;
                }
                given[prop.name.toString().toStdString()] = static_cast<double> (prop.value);
            }
    }

    abmap::Options opt;
    if (const juce::var o = params.getProperty ("options", juce::var()); o.isObject())
    {
        opt.dynMatchProtoTimeWiring = static_cast<bool> (o.getProperty ("dynMatchProtoTimeWiring", false));
        opt.shelfGainNominal = static_cast<bool> (o.getProperty ("shelfGainNominal", false));
        opt.allowUnmapped = static_cast<bool> (o.getProperty ("allowUnmapped", false));
    }

    if (cfg.sr <= 0.0)
        cfg.sr = static_cast<double> (params.getProperty ("sampleRate", juce::var (0.0)));
    if (params.hasProperty ("block") && cfg.block == 512)
        cfg.block = static_cast<int> (params.getProperty ("block", juce::var (512)));

    if (cfg.block <= 0)
    {
        std::fprintf (stderr, "error: --block must be positive\n");
        return 2;
    }

    //---------------------------------------------------------------- input
    const juce::File inFile = juce::File::getCurrentWorkingDirectory().getChildFile (cfg.inPath);
    std::vector<float> input;
    double fileRate = 0.0;
    juce::String err;

    if (! readWav (inFile, input, fileRate, err))
    {
        std::fprintf (stderr, "error: %s\n", err.toRawUTF8());
        std::fprintf (stderr, "hint: run  python gen_input.py  first\n");
        return 3;
    }

    const std::string inSha = fingerprint (input);

    if (cfg.expectSha.isNotEmpty() && cfg.expectSha != juce::String (inSha))
    {
        std::fprintf (stderr, "error: input fingerprint mismatch\n  expected %s\n  got      %s\n",
                      cfg.expectSha.toRawUTF8(), inSha.c_str());
        return 6;
    }

    const double sampleRate = cfg.sr > 0.0 ? cfg.sr : fileRate;
    if (cfg.sr > 0.0 && std::fabs (cfg.sr - fileRate) > 0.5)
        std::fprintf (stderr, "warning: --sr %.0f overrides the file's %.0f Hz; the delay and\n"
                              "         chorus/flanger mappings are rate dependent\n", cfg.sr, fileRate);

    //-------------------------------------------------------------- mapping
    const abmap::MapResult mapped = abmap::map (*patch, given, sampleRate, opt);

    std::printf ("ab-render  %s -> module '%s' (%s)\n", patch->name, patch->module,
                 patch->nonlinear ? "nonlinear" : "linear");
    std::printf ("  input  %s\n", inFile.getFullPathName().toRawUTF8());
    std::printf ("         %d samples at %.0f Hz, sha256 %s\n",
                 static_cast<int> (input.size()), fileRate, inSha.c_str());
    std::printf ("  rate   %.0f Hz, block %d\n", sampleRate, cfg.block);
    std::printf ("  opts   dynMatchProtoTimeWiring=%s  shelfGainNominal=%s  allowUnmapped=%s\n\n",
                 opt.dynMatchProtoTimeWiring ? "true" : "false",
                 opt.shelfGainNominal ? "true" : "false",
                 opt.allowUnmapped ? "true" : "false");

    std::printf ("  resolved inlets\n");
    for (const auto& line : mapped.resolved)
        std::printf ("    %s\n", line.c_str());

    if (! mapped.blockers.empty())
    {
        std::fprintf (stderr, "\nREFUSED - these parameters cannot be compared:\n");
        for (const auto& b : mapped.blockers)
            std::fprintf (stderr, "  * %s\n", b.c_str());
        return 5;
    }

    std::printf ("\n  our parameters\n");
    for (const auto& line : mapped.native)
        std::printf ("    %s\n", line.c_str());

    if (! mapped.warnings.empty())
    {
        std::printf ("\n  warnings\n");
        for (const auto& w : mapped.warnings)
            std::printf ("    ! %s\n", w.c_str());
    }

    //--------------------------------------------------------------- render
    spatcore::effects::ChainConfig chainCfg;
    chainCfg.sampleRate = sampleRate;
    chainCfg.maxBlock = cfg.block;
    chainCfg.maxEffectDelaySeconds = 5.0;
    chainCfg.noiseKey = 1;

    // Which instance of the module type this slot is: dyn2 and eq2 are the only
    // doubled ones, and no prototype maps onto the second of either.
    const int instance = 0;

    auto module = spatcore::effects::createModule (patch->id, instance, chainCfg);
    if (module == nullptr)
    {
        std::fprintf (stderr, "error: no module for %s\n", patch->name);
        return 2;
    }

    module->prepare (chainCfg);

    // The first applyParams after prepare() SNAPS every smoother, so the render
    // starts settled instead of gliding in from a default the user never chose.
    // It is called once and never again: nothing automates in an A/B.
    const spatcore::effects::ParamApplyInfo info = module->applyParams (mapped.params, instance);
    if (info.bypass)
    {
        std::fprintf (stderr, "error: the module reported itself bypassed - the mapping did not\n"
                              "       clear the bypass flag, which is a bug in ab_mapping.h\n");
        return 2;
    }
    if (info.variantPending)
        module->commitPendingVariant();

    std::vector<float> output (input);
    const int total = static_cast<int> (output.size());
    for (int offset = 0; offset < total; offset += cfg.block)
    {
        const int n = std::min (cfg.block, total - offset);
        module->process (output.data() + offset, n);
    }

    const int latency = module->getLatencySamples();

    //--------------------------------------------------------------- output
    const juce::File outFile = juce::File::getCurrentWorkingDirectory().getChildFile (cfg.outPath);
    if (! writeWav (outFile, output, sampleRate))
    {
        std::fprintf (stderr, "error: cannot write %s\n", outFile.getFullPathName().toRawUTF8());
        return 4;
    }

    const juce::File sidecar = cfg.sidecarPath.isNotEmpty()
                                   ? juce::File::getCurrentWorkingDirectory().getChildFile (cfg.sidecarPath)
                                   : outFile.withFileExtension ("map.json");

    writeSidecar (sidecar, cfg, *patch, mapped, opt, input, output,
                  sampleRate, latency, inSha, inFile, outFile);

    double peak = 0.0, sum = 0.0;
    for (const float s : output)
    {
        const double a = std::fabs (static_cast<double> (s));
        if (a > peak) peak = a;
        sum += static_cast<double> (s) * static_cast<double> (s);
    }
    const double rms = total > 0 ? std::sqrt (sum / static_cast<double> (total)) : 0.0;

    std::printf ("\n  rendered %d samples, module latency %d sample%s\n",
                 total, latency, latency == 1 ? "" : "s");
    std::printf ("           peak %.6f (%.2f dBFS), rms %.2f dBFS\n",
                 peak, peak > 0.0 ? 20.0 * std::log10 (peak) : -999.0,
                 rms > 0.0 ? 20.0 * std::log10 (rms) : -999.0);
    std::printf ("  wrote    %s\n", outFile.getFullPathName().toRawUTF8());
    std::printf ("           %s\n", sidecar.getFullPathName().toRawUTF8());

    if (! mapped.active.empty())
    {
        std::printf ("\n  EXPECTED DIFFERENCES in force for this parameter set\n");
        std::printf ("  (ab_compare.py downgrades the metrics they touch from FAIL to\n"
                     "   EXPECTED-DIFF; these are the ghosts you must not chase)\n\n");

        printDiffs (mapped.active);
    }

    if (peak > 1.0)
        std::printf ("  note: our render peaks above full scale. If the Max side is exported\n"
                     "        as integer PCM it will CLIP where this does not - export float.\n");

    return 0;
}
