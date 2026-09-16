#pragma once

//==============================================================================
// ab_mapping.h - gen~ prototype inlets -> spatcore::effects::EffectChannelParams
//
// This file is the reason the harness exists in the form it does. The seven
// prototypes in Documentation/effects/ and our nine modules do NOT share a
// convention, and every one of the mismatches is silent: a number that means
// something different on each side still renders, still produces a plausible
// waveform, and still yields a residual - just one that measures the wrong
// thing. The conversions are therefore encoded here and nowhere else, and
// ab_render refuses an inlet name it does not recognise rather than ignoring
// it, so a typo cannot quietly become a default.
//
// The conventions that differ (verified by the Phase 2 re-decode, 2026-09-16):
//
//   * dry/wet    wetDry.gendsp computes out = dry*m + wet*(1-m) with m =
//                inlet/100. The inlet is therefore a DRY per cent. Every
//                effect*Mix of ours is a WET per cent: mix = 100 - inlet.
//                fx_tremolo is the exception - it does its own arithmetic, its
//                drywet is a WET FRACTION 0..1, and mix = inlet * 100.
//   * distortion the `waveform` inlet is a per cent of the HARD CLIPPER
//                (100 = pure clip, 0 = pure tanh). effectDistShape runs the
//                other way (0 = clip, 1 = tanh): shape = 1 - inlet/100.
//   * times      fx_delay's delayTime is in SECONDS with a 1 ms floor; ours is
//                in ms. fx_chorus and fx_flanger state their delay in SAMPLES,
//                converted here at the render rate. fx_dynamics is already ms.
//   * feedback   fx_flanger multiplies a per cent by a raw polarity inlet;
//                fx_chorus has NO feedback amount at all, only a three-state
//                polarity worth exactly 0, +1 or -1. Ours is a signed per cent.
//   * shelf gain the shelf sub-patches run the gain through dbtoa BEFORE a
//                codebox that treats it as dB (plan section 5.13, P2), so the
//                realised shelf gain is 10^(inlet/20) dB. The mapping converts
//                by default, which puts the same shelf on both sides and leaves
//                P1 (the alpha bug) as the only shelf difference. -200 on the
//                inlet is the prototype's neutral, not 0.
//   * frequency  fx_chorus/fx_flanger/fx_delay state cut and shelf corners in
//                Hz, as we do - but at defaults (1 Hz shelves) that our modules
//                clamp away. The clamp is reported, never silently applied.
//
// Expected differences: section 5.13 of Documentation/effects-channels-plan.md
// records four DEFECTS in the prototypes that the modules deliberately do not
// reproduce, plus this file carries the further verified divergences. On the
// parameters they touch the correct outcome is a LARGE residual. Each Patch
// below therefore declares which differences are in force for the parameter
// set actually being rendered, ab_render writes them into the sidecar, and
// ab_compare.py downgrades an affected metric from FAIL to EXPECTED-DIFF. A
// harness without this would tell the user to break working code.
//==============================================================================

#include "../../../spatcore/effects/EffectParams.h"
#include "../../../spatcore/dsp/LFOWaveforms.h"

#include <cmath>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace abmap
{

using spatcore::effects::EffectChannelParams;
using spatcore::effects::ModuleId;

//==============================================================================
/** One gen~ inlet, as the patch declares it. */
struct Inlet
{
    const char* name;           ///< the @comment name, exactly as the box spells it
    int index;                  ///< the gen~ inlet number (1-based; 1 is the signal)
    double defaultValue;        ///< the box's @default, or gen~'s implicit 0
    const char* unit;           ///< Hz / dB / ms / s / samples / % / fraction / enum
    const char* note;           ///< how it converts, and anything surprising about it
};

/** A difference we EXPECT to see, with what the user can do about it. */
struct Diff
{
    const char* id;             ///< P1..P4 from plan section 5.13, or D-* for the rest
    const char* severity;       ///< "large" (do not chase) or "small" (know about it)
    const char* metrics;        ///< comma-separated: rms, spectral, modRate, modDepth
    const char* title;
    const char* why;
    const char* remedy;         ///< what to do in Max, or "" when nothing can be done
};

/** Options that change the mapping itself, declared in params.json. */
struct Options
{
    /** fx_dynamics only. The prototype wires Cattack to slide-UP and Crelease
        to slide-DOWN on a LINEAR gain that falls when compressing, so its
        Cattack is really the release and its Crelease is really the attack
        (verified; the expander beside it uses the same wiring, where it is
        correct by gate convention, which is what makes the compressor side a
        bug rather than a convention). The mapping follows the INTENT by
        default - Cattack becomes effectDynCompAttack - and declares the
        difference. Set this true to follow the WIRING instead and compare two
        identical envelopes. */
    bool dynMatchProtoTimeWiring = false;

    /** Shelf GAIN inlets. loShelf.gendsp / hiShelf.gendsp pass the inlet through
        a `dbtoa` box and the codebox then treats the result as dB (plan section
        5.13, defect P2), so the prototype's effective shelf gain is
        10^(inlet/20) dB - a nominal 0 dB shelf really applies +1.00 dB, and a
        nominal +40 dB asks for +100 dB. By default the mapping converts, so
        both sides run the SAME shelf and only the separate alpha defect (P1)
        is left to show up in the metrics. Set this true to send the nominal
        number straight through instead and see the raw knob-for-knob
        difference, which is what an operator turning both controls to the same
        number would get. */
    bool shelfGainNominal = false;

    /** Render anyway when a prototype control has no counterpart in our module
        (currently: fx_chorus's second delay line). Off by default, because the
        resulting comparison measures nothing. */
    bool allowUnmapped = false;
};

/** What a conversion produced. */
struct MapResult
{
    EffectChannelParams params;             ///< everything bypassed except the module
    std::string module;                     ///< the slot token: trem, dist, ...
    ModuleId moduleId = ModuleId::Count;
    bool nonlinear = false;                 ///< picks the -30 dB target over -40 dB

    std::vector<std::string> resolved;      ///< "inlet = value (source)" per inlet
    std::vector<std::string> native;        ///< "field = value" per converted field
    std::vector<std::string> warnings;      ///< clamps hit, values out of our range
    std::vector<const Diff*> active;        ///< expected differences in force
    std::vector<std::string> blockers;      ///< refusals; non-empty means do not render
};

//==============================================================================
namespace detail
{
    inline float f (double v) { return static_cast<float> (v); }

    inline double clampRep (MapResult& r, const char* field, double v, double lo, double hi)
    {
        if (v < lo || v > hi)
        {
            const double c = v < lo ? lo : hi;
            char buf[256];
            std::snprintf (buf, sizeof (buf),
                           "%s: the prototype value %.6g is outside our range %.6g..%.6g "
                           "and was clamped to %.6g - the two renders are NOT running the "
                           "same setting on this control",
                           field, v, lo, hi, c);
            r.warnings.emplace_back (buf);
            return c;
        }
        return v;
    }

    inline void rec (MapResult& r, const char* field, double v)
    {
        char buf[160];
        std::snprintf (buf, sizeof (buf), "%s = %.6g", field, v);
        r.native.emplace_back (buf);
    }

    inline double get (const std::map<std::string, double>& in, const char* key, double dflt)
    {
        const auto it = in.find (key);
        return it == in.end() ? dflt : it->second;
    }

    /** The shelf gain the prototype's biquad actually realises, in dB, for a
        given value on its gain inlet: `dbtoa` runs before a codebox that
        computes A = 10^(in/40), and an RBJ shelf's asymptotic gain is A^2, so
        the realised gain in dB is exactly the dbtoa output. Verified against
        the coefficients: inlet 0 dB -> +1.000 dB, 6 dB -> +1.995 dB,
        -40 dB -> +0.010 dB, -200 dB -> 0.000 dB. */
    inline double protoShelfDb (double inletDb)
    {
        return std::pow (10.0, inletDb / 20.0);
    }

    /** Below this the prototype's shelf is the identity to within 0.01 dB, so
        the corner frequency stops mattering too. */
    inline bool shelfIsNeutral (double inletDb) { return inletDb <= -40.0; }

    inline double wrap360 (double d)
    {
        while (d < 0.0)      d += 360.0;
        while (d >= 360.0)   d -= 360.0;
        return d;
    }
} // namespace detail

//==============================================================================
// The differences. Plan section 5.13's four defects first, then the divergences
// this harness found while writing the mapping.
//==============================================================================

inline constexpr Diff kP1
{
    "P1", "large", "rms,spectral",
    "Shelf alpha is missing a pair of brackets",
    "loShelf.gendsp / hiShelf.gendsp compute alpha = sin(w/2)*sqrt((A + 1/A)/S + 1) where "
    "RBJ is sqrt((A + 1/A)(1/S - 1) + 2). At a nominal S = 0.7 the prototype is really "
    "running S = 0.52: 0.19 dB of extra shelf at +-3 dB of gain, 2.07 dB at +-24 dB. "
    "The modules implement the correct law on purpose (plan section 5.13, P1).",
    "Measured here against the corrected law at S = 0.7 and a 200 Hz corner: 0.06 dB at "
    "a 1 dB shelf, 0.19 dB at 3 dB, 0.39 dB at 6 dB, 0.86 dB at 12 dB, 2.07 dB at 24 dB. "
    "To take the shelves out of the comparison entirely send -200 to every shelf GAIN "
    "inlet: the prototype's A then collapses to 1 and an RBJ shelf at A = 1 is the "
    "identity whatever its alpha, and this mapping sets our own shelf to 0 dB, which the "
    "module switches off. A NOMINAL 0 dB does not do that on either side."
};

inline constexpr Diff kP2
{
    "P2", "large", "rms,spectral",
    "Shelf gain is converted from dB twice, and this run did not convert it back",
    "The gain inlet passes through a dbtoa box and the codebox then computes A = "
    "10^(in2/40) as though the result were still dB, so the prototype's realised shelf "
    "gain is 10^(inlet/20) dB: a nominal 0 dB shelf applies +1.00 dB, 6 dB applies "
    "+1.995 dB, and a nominal +40 dB asks for +100 dB (plan section 5.13, P2 - its "
    "'+0.5 dB' figure is A expressed in dB; the shelf's asymptotic gain is A^2, which "
    "the coefficients give as +1.00 dB). This run is in \"shelfGainNominal\" mode, so the "
    "same number went to both sides and the shelves are genuinely different.",
    "Drop \"shelfGainNominal\" from the options (it is off by default) and the mapping "
    "converts for you, leaving only P1."
};

inline constexpr Diff kP3
{
    "P3", "large", "rms,spectral",
    "The expander realises the wrong ratio",
    "fx_dynamics computes the expander gain with the COMPRESSOR's slope expression, "
    "1 - 1/R, so the realised transfer is out_dB = T + (L - T)*(2 - 1/R): 1.5:1 at a "
    "nominal 2:1, and it saturates at 2:1 however large R gets. It cannot gate. "
    "DynamicsModule implements the correct downward expander (plan section 5.13, P3).",
    "There is no inlet value that fixes this - the prototype's realisable range stops "
    "at 2:1. Send Xratio = 1 to take the expander out of the comparison entirely."
};

inline constexpr Diff kP4
{
    "P4", "large", "rms,spectral",
    "A stale sidechain default that mutes the detector",
    "hiCut.gendsp's second inlet still carries the label and @default of a high SHELF, "
    "and fx_dynamics's own CscHiFreq / XscHiFreq boxes repeat it: both sidechain high "
    "cuts default to 20 Hz rather than 20 kHz, so the detector sees essentially nothing "
    "and the stage barely responds. Ours ships 20 kHz (plan section 5.13, P4).",
    "Send 20000 to CscHiFreq and to XscHiFreq in Max. This one is entirely removable "
    "from the Max side, so do it before reading anything into a dynamics A/B."
};

inline constexpr Diff kTremWetLeg
{
    "D-TREM-WETLEG", "large", "rms,spectral,modDepth",
    "The prototype's tremolo wet leg subtracts instead of scaling",
    "fx_tremolo builds out = in*(1-w) + in*w*(g-1). The `- 1` after dbtoa is a bug: at "
    "w = 1 the prototype emits in*(g-1), which is inverted and near-silent because the "
    "modulation gain g is always <= 1. TremoloModule implements in*((1-w) + w*g). The "
    "modulation RATE still measures correctly on both sides; the depth and the residual "
    "do not (plan section 5.12, quirks not carried over).",
    "Nothing can be done from the Max side without editing the patch. Read the modRate "
    "metric and ignore rms, spectral and modDepth for this module."
};

inline constexpr Diff kCrushDither
{
    "D-CRUSH-DITHER", "large", "rms,spectral",
    "Dither noise cannot match sample for sample",
    "The prototype adds Max's `noise` (its own generator, seeded by the runtime); "
    "BitcrusherModule adds FrDiffusion::hashNoiseBipolar so a render is reproducible "
    "across platforms. Two different noise streams at the same level give a residual at "
    "roughly the dither level plus 3 dB, whatever else is right.",
    "Send -96 or lower to the dither inlet. The gen~ @default of 0 dB is FULL-SCALE "
    "noise, which is a placeholder rather than a setting (plan section 5.9)."
};

inline constexpr Diff kCrushNoRate
{
    "D-CRUSH-NORATE", "small", "",
    "The prototype has no sample-rate reduction",
    "fx_bitcrusher is a quantiser and a dither, nothing else - there is no "
    "sample-and-hold in the patch. effectCrushRate is pinned to the render rate for "
    "this comparison and effectCrushFilter to 0, so the rate half of our module is "
    "simply out of the A/B's scope.",
    ""
};

inline constexpr Diff kDynTimes
{
    "D-DYN-TIMES", "large", "rms",
    "The prototype's compressor attack and release are swapped",
    "Cattack drives slide-UP and Crelease slide-DOWN on a LINEAR gain that FALLS when "
    "compressing, so the prototype grabs over Crelease and lets go over Cattack - "
    "backwards. The expander beside it uses the same wiring, where it is correct by gate "
    "convention, which is what makes this a bug rather than a house style. The mapping "
    "follows the intent by default.",
    "Set \"options\": { \"dynMatchProtoTimeWiring\": true } to map Cattack to our release "
    "and Crelease to our attack, and the two envelopes then match. Or send the two "
    "numbers swapped in Max."
};

inline constexpr Diff kDynDetDelay
{
    "D-DYN-DETDELAY", "small", "rms",
    "The prototype's detector delay truncates and caps",
    "fx_dynamics's detector delay is a BARE gen~ `delay` - the only one in the folder "
    "without an explicit maximum - so it takes gen~'s 512-sample default (about 10.7 ms "
    "at 48 kHz), does not interpolate, truncates toward zero, and still delays by one "
    "sample at a setting of 0. Ours rounds to the nearest sample and has the full range.",
    "Send CdetectionDelay = 0 to remove all but the prototype's one-sample floor."
};

inline constexpr Diff kDynSlide
{
    "D-DYN-SLIDE", "small", "rms",
    "gen~'s slide coefficient is not ours",
    "`slide` uses coef = 1/n for n = t_ms * sr/1000; EnvelopeFollower uses "
    "1 - exp(-1/n). They agree to first order and diverge about 10 % at the shortest "
    "times (n around 5 samples), which is audible on a fast attack and negligible above "
    "roughly 1 ms.",
    ""
};

inline constexpr Diff kDynStartup
{
    "D-DYN-STARTUP", "small", "rms",
    "The prototype fades in from silence",
    "gen~'s slide histories start at 0 and the smoothed quantity is the linear gain, so "
    "both stages ramp 0 -> 1 over their attack time at the start of a render. Ours starts "
    "settled. The input file's first 0.20 s are silent, which covers it - but only if the "
    "Max render starts at the same sample.",
    ""
};

inline constexpr Diff kChorusFeedback
{
    "D-CHORUS-FB", "large", "rms,spectral",
    "The prototype's chorus feedback is exactly unity",
    "fx_chorus has no feedback AMOUNT inlet at all: feedbackPolarity is decoded to a "
    "coefficient of exactly 0, +1 or -1, so switching feedback on gives a comb that never "
    "decays. ModulationModule clamps to +-95 % on purpose (plan section 5.4, quirks not "
    "carried over).",
    "Send feedbackPolarity = 0 to compare the chorus without its feedback path."
};

inline constexpr Diff kChorusLine2
{
    "D-CHORUS-LINE2", "large", "rms,spectral",
    "The prototype's second delay line has no counterpart",
    "fx_chorus runs two independently timed lines: delaytime2 is entered divided by 100, "
    "gated by delaytime2onOff (a bare multiply, so it doubles as a level), and modulated "
    "by its own delaytime2LFO factor. ModulationModule's `voices` spreads ONE line's LFO "
    "phase, which is a different thing.",
    "Send delaytime2onOff = 0. With it non-zero the comparison measures nothing, so the "
    "render is refused unless \"options\": { \"allowUnmapped\": true } is set."
};

inline constexpr Diff kModFbFilter
{
    "D-MOD-FBFILTER", "small", "rms,spectral",
    "Our modulation feedback path is filtered",
    "ModulationModule puts a DC blocker and a high cut in the feedback loop so a long "
    "session cannot wind up on DC; the prototypes recirculate the raw tap. With feedback "
    "at 0 the two paths are identical.",
    "Send the feedback (or the polarity, on fx_chorus) to 0 to take this out."
};

inline constexpr Diff kModSamples
{
    "D-MOD-SAMPLES", "small", "",
    "The delay time is in SAMPLES, converted at the render rate",
    "fx_chorus and fx_flanger state their delay in samples - there is no mstosamps "
    "anywhere in either patch - so the same patch is a different delay at a different "
    "rate. The mapping converts at the rate this render uses; render the Max side at the "
    "same rate or the comparison is meaningless.",
    ""
};

inline constexpr Diff kModMode
{
    "D-MOD-MODE", "small", "",
    "effectModMode does not currently select an algorithm",
    "ModulationModule reads delayMs, depth and feedback but not `mode`, so chorus and "
    "flanger differ only in their settings. The field is set here for the record.",
    ""
};

inline constexpr Diff kDelayShelves
{
    "D-DELAY-SHELVES", "large", "rms,spectral",
    "The prototype's delay shelves sit inside the feedback loop and are never neutral",
    "fx_delay's two shelves are after the feedback gain and inside the loop, and both "
    "default to a 1 Hz corner, so they act broadband. Through P2 a nominal 0 dB shelf "
    "applies about +0.5 dB - so the prototype's loop gains roughly 1 dB per repeat that "
    "ours does not, and the decay times diverge more with every repeat. Our module "
    "switches a 0 dB shelf off entirely rather than running a biquad at nominal unity "
    "inside a feedback loop.",
    "Drop \"shelfGainNominal\" so the mapping converts the gain for you, or send -200 to "
    "loShelfGain and hiShelfGain to take the shelves out of the loop on both sides."
};

inline constexpr Diff kDelayShelfRange
{
    "D-DELAY-SHELFRANGE", "small", "spectral",
    "The prototype's 1 Hz shelf corners are outside our range",
    "MultitapDelayModule clamps the feedback shelves to 20..2000 Hz and 1000..20000 Hz. "
    "The prototype's @default of 1 Hz cannot be reproduced. It only matters while the "
    "shelf gains are live.",
    "Send a corner inside our range, or neutralise the gains (-200 on the inlet) and the "
    "corner stops mattering."
};

inline constexpr Diff kDelayModPhase
{
    "D-DELAY-MODPHASE", "large", "rms,modDepth",
    "The delay LFOs are a quarter cycle apart and cannot be aligned",
    "fx_delay modulates with `cycle` (a COSINE); MultitapDelayModule's LFO is a sine, and "
    "unlike ModulationModule it has no phase parameter. Two identical modulations 90 "
    "degrees apart produce a residual comparable to the modulation itself.",
    "Send modDepth = 0 (the gen~ default) and compare the delay without modulation."
};

inline constexpr Diff kDelayFirstRepeat
{
    "D-DELAY-FIRSTREPEAT", "small", "",
    "The first repeat is at unity on both sides",
    "The prototype writes its line at unity and scales only the recirculating path, so "
    "the first repeat ignores the feedback control. MultitapDelayModule does the same "
    "(tap gain then feedback on the recirculation), so this one MATCHES - recorded "
    "because it is the kind of thing a reader assumes is a bug.",
    ""
};

//==============================================================================
// The patches.
//==============================================================================

inline const Inlet kTremoloInlets[] =
{
    { "freq",     2, 0.0, "Hz",       "straight to effectTremRate" },
    { "waveform", 3, 0.0, "fraction", "0 = the cosine leg, 1 = the triangle leg; effectTremShape 0..1 is the same blend and the two legs are phase aligned on both sides" },
    { "depth",    4, 0.0, "dB",       "the modulation is dB-linear on both sides; straight to effectTremDepth" },
    { "drywet",   5, 0.0, "fraction", "THE EXCEPTION: fx_tremolo does its own dry/wet and this is a WET FRACTION 0..1, not a dry per cent - effectTremMix = drywet * 100" },
};

inline const Inlet kBitcrusherInlets[] =
{
    { "resolution", 2, 24.0, "bits", "q(x) = round(x * 2^bits)/2^bits on both sides; straight to effectCrushBits" },
    { "dither",     3,  0.0, "dB",   "noise level before the quantiser; the @default of 0 dB is FULL SCALE. effectCrushDither, with -96 meaning off" },
    { "drywet",     4,  0.0, "%",    "a DRY per cent through wetDry.gendsp: effectCrushMix = 100 - drywet" },
};

inline const Inlet kDistortionInlets[] =
{
    { "drive",              2,     0.0, "dB", "through dbtoa into the drive multiply; straight to effectDistDrive" },
    { "waveform",           3,     0.0, "%",  "INVERTED: a per cent of the HARD CLIPPER (100 = pure clip, 0 = pure tanh). effectDistShape = 1 - waveform/100" },
    { "outputAttenuation",  4,     0.0, "dB", "despite the name a positive value BOOSTS; straight to effectDistOutput" },
    { "dryWet",             5,     0.0, "%",  "a DRY per cent: effectDistMix = 100 - dryWet" },
    { "loShelfFreq-pre",    6,    20.0, "Hz", "effectDistPreLoShelfFreq" },
    { "loShelfGain-pre",    7,     0.0, "dB", "effectDistPreLoShelfGain = 10^(inlet/20) dB - the prototype converts from dB twice (P2), so a nominal 0 dB shelf really applies +1.00 dB" },
    { "hiShelfFreq-pre",    8, 20000.0, "Hz", "effectDistPreHiShelfFreq" },
    { "hiShelfGain-pre",    9,     0.0, "dB", "effectDistPreHiShelfGain = 10^(inlet/20) dB - see P2" },
    { "loShelfFreq-post",  10,     1.0, "Hz", "effectDistPostLoShelfFreq; the @default of 1 Hz is a placeholder and is below our 20 Hz floor" },
    { "loShelfGain-post",  11,     0.0, "dB", "effectDistPostLoShelfGain = 10^(inlet/20) dB - see P2" },
    { "hiShelfFreq-post",  12,     1.0, "Hz", "effectDistPostHiShelfFreq; a HIGH shelf at 1 Hz acts broadband, so with P2 it is a flat +0.5 dB" },
    { "hiShelfGain-post",  13,     0.0, "dB", "effectDistPostHiShelfGain = 10^(inlet/20) dB - see P2" },
};

inline const Inlet kDynamicsInlets[] =
{
    { "Cthreshold",      2,   0.0, "dB", "effectDynCompThreshold" },
    { "Cratio",          3,   1.0, "",   "effectDynCompRatio; the compressor's law is textbook-correct on both sides" },
    { "Cattack",         4,  10.0, "ms", "effectDynCompAttack by INTENT; the prototype wires it to slide-up, i.e. its release - see D-DYN-TIMES" },
    { "Crelease",        5, 100.0, "ms", "effectDynCompRelease by intent - see D-DYN-TIMES" },
    { "CscLoFreq",       6,  20.0, "Hz", "effectDynCompScLoCut; loCut.gendsp and OutputEQBiquadFilter shape 1 at q = 0.6 agree to the bit" },
    { "CscHiFreq",       7,  20.0, "Hz", "effectDynCompScHiCut; the @default of 20 is the stale hiShelf label - see P4" },
    { "CdetectionDelay", 8,   0.0, "ms", "effectDynCompDetectorDelay; delays the DETECTOR, the opposite of lookahead" },
    { "Xthreshold",      9,   0.0, "dB", "effectDynExpThreshold" },
    { "Xratio",         10,   1.0, "",   "effectDynExpRatio; the prototype realises 2 - 1/R instead - see P3" },
    { "Xattack",        11,  10.0, "ms", "effectDynExpAttack; correct by gate convention on both sides" },
    { "Xrelease",       12,  10.0, "ms", "effectDynExpRelease" },
    { "XscLoFreq",      13,  20.0, "Hz", "effectDynExpScLoCut" },
    { "XscHiFreq",      14,  20.0, "Hz", "effectDynExpScHiCut - see P4" },
    { "gain",           15,   0.0, "dB", "one makeup multiply after both stages; effectDynMakeup" },
};

inline const Inlet kChorusInlets[] =
{
    { "delaytime1",      2,  0.0, "samples", "SAMPLES, not ms: effectModDelay = delaytime1 / sampleRate * 1000" },
    { "delaytime2",      3,  0.0, "samples", "the second line, entered divided by 100 - no counterpart in our module" },
    { "delaytime2onOff", 4,  0.0, "gain",    "a bare multiply on line 2's wet tap, so it doubles as a level; must be 0 to compare" },
    { "delaytime2LFO",   5,  0.0, "x",       "line 2's own modulation multiplier - no counterpart" },
    { "LFOspeed",        6,  0.0, "Hz",      "into `cycle`, a COSINE; effectModRate, with effectModPhase set to 180 to align our -cos" },
    { "LFOamount",       7,  0.0, "%",       "a per cent of the centre delay, the same definition as effectModDepth" },
    { "feedbackPolarity",8,  0.0, "enum",    "3-state: 0 = off, exactly 2 = inverted, anything else non-zero = positive. The coefficient is exactly +-1 - see D-CHORUS-FB" },
    { "loCutFreq",       9, 20.0, "Hz",      "effectModLoCut; the only inlet in the file with an @default" },
    { "drywet",         10,  0.0, "%",       "a DRY per cent: effectModMix = 100 - drywet" },
};

inline const Inlet kFlangerInlets[] =
{
    { "delaytime",       2,  0.0, "samples", "SAMPLES: effectModDelay = delaytime / sampleRate * 1000" },
    { "feedback",        3,  0.0, "%",       "multiplied by feedbackPolarity to give our signed effectModFeedback" },
    { "feedbackPolarity",4,  0.0, "x",       "a RAW multiplier here, not a switch - 0, -1, +1 and 2 are all accepted" },
    { "LFOamount",       5,  0.0, "%",       "a per cent of the delay time; effectModDepth" },
    { "LFOspeed",        6,  0.0, "Hz",      "into `phasor`, then sin(); effectModRate" },
    { "LFOphase",        7,  0.0, "degrees", "effectModPhase = LFOphase + 90: the prototype is sin(2pi(p + ph/360)) and ours is -cos(2pi(p + off))" },
    { "loCutFreq",       8, 20.0, "Hz",      "effectModLoCut" },
    { "drywet",          9,  0.0, "%",       "a DRY per cent: effectModMix = 100 - drywet" },
};

inline const Inlet kDelayInlets[] =
{
    { "delayTime",    2, 0.0, "s",  "SECONDS, floored at 1 ms by a `max 1` after the *1000: effectDelayTime = max(1, delayTime * 1000) ms" },
    { "modFreq",      3, 0.1, "Hz", "effectDelayModRate" },
    { "modDepth",     4, 0.0, "fraction", "a raw FRACTION of the delay time, not a per cent: effectDelayModDepth = modDepth * 100" },
    { "loCutFreq",    5, 20.0, "Hz", "effectDelayInLoCut; the input cut is outside the feedback loop on both sides" },
    { "loShelfFreq",  6,  1.0, "Hz", "effectDelayFbLoShelfFreq; inside the loop. The @default of 1 Hz is below our 20 Hz floor" },
    { "loShelfGain",  7,  0.0, "dB", "effectDelayFbLoShelfGain = 10^(inlet/20) dB (P2); a nominal 0 dB shelf applies +1.00 dB PER REPEAT inside the prototype's loop" },
    { "hiShelfFreq",  8,  1.0, "Hz", "effectDelayFbHiShelfFreq; below our 1000 Hz floor" },
    { "hiShelfGain",  9,  0.0, "dB", "effectDelayFbHiShelfGain = 10^(inlet/20) dB - see P2" },
    { "feedback",    10,  0.0, "%",  "effectDelayFeedback; unsigned on both sides. The FIRST repeat is at unity regardless, on both sides" },
    { "dryWet",      11,  0.0, "%",  "a DRY per cent: effectDelayMix = 100 - dryWet" },
};

//==============================================================================
// Conversions.
//==============================================================================

using Values = std::map<std::string, double>;

inline void convertTremolo (const Values& in, double, const Options&, MapResult& r)
{
    using namespace detail;
    auto& t = r.params.trem;
    t.bypass = 0;

    const double freq   = get (in, "freq", 0.0);
    const double wave   = get (in, "waveform", 0.0);
    const double depth  = get (in, "depth", 0.0);
    const double drywet = get (in, "drywet", 0.0);

    t.rateHz  = f (clampRep (r, "effectTremRate",  freq,  0.05, 20.0));
    t.shape   = f (clampRep (r, "effectTremShape", wave,  0.0,  1.0));
    t.depthDb = f (clampRep (r, "effectTremDepth", depth, 0.0,  60.0));
    t.mix     = f (clampRep (r, "effectTremMix",   drywet * 100.0, 0.0, 100.0));

    rec (r, "trem.rateHz", t.rateHz);
    rec (r, "trem.shape", t.shape);
    rec (r, "trem.depthDb", t.depthDb);
    rec (r, "trem.mix (= drywet * 100, a WET fraction)", t.mix);

    if (depth > 0.0 && drywet > 0.0)
        r.active.push_back (&kTremWetLeg);
}

inline void convertBitcrusher (const Values& in, double sr, const Options&, MapResult& r)
{
    using namespace detail;
    auto& c = r.params.crush;
    c.bypass = 0;
    c.filter = 0;

    const double bits   = get (in, "resolution", 24.0);
    const double dither = get (in, "dither", 0.0);
    const double drywet = get (in, "drywet", 0.0);

    c.bits     = f (clampRep (r, "effectCrushBits", bits, 1.0, 24.0));
    c.rateHz   = f (sr);                                // no decimation in the prototype
    c.ditherDb = f (clampRep (r, "effectCrushDither", dither, -96.0, 0.0));
    c.mix      = f (clampRep (r, "effectCrushMix", 100.0 - drywet, 0.0, 100.0));

    rec (r, "crush.bits", c.bits);
    rec (r, "crush.rateHz (pinned to the render rate)", c.rateHz);
    rec (r, "crush.filter", c.filter);
    rec (r, "crush.ditherDb", c.ditherDb);
    rec (r, "crush.mix (= 100 - drywet)", c.mix);

    r.active.push_back (&kCrushNoRate);
    if (dither > -96.0)
        r.active.push_back (&kCrushDither);
}

inline void convertDistortion (const Values& in, double, const Options& opt, MapResult& r)
{
    using namespace detail;
    auto& d = r.params.dist;
    d.bypass = 0;
    d.oversample = 1;                                   // off: the prototype does not
    d.bias = 0.0f;                                      // no prototype counterpart

    const double drive = get (in, "drive", 0.0);
    const double wave  = get (in, "waveform", 0.0);
    const double out   = get (in, "outputAttenuation", 0.0);
    const double mix   = get (in, "dryWet", 0.0);

    d.driveDb  = f (clampRep (r, "effectDistDrive", drive, 0.0, 40.0));
    d.shape    = f (clampRep (r, "effectDistShape", 1.0 - wave / 100.0, 0.0, 1.0));
    d.outputDb = f (clampRep (r, "effectDistOutput", out, -24.0, 12.0));
    d.mix      = f (clampRep (r, "effectDistMix", 100.0 - mix, 0.0, 100.0));

    const double loPreHz  = get (in, "loShelfFreq-pre", 20.0);
    const double loPreDb  = get (in, "loShelfGain-pre", 0.0);
    const double hiPreHz  = get (in, "hiShelfFreq-pre", 20000.0);
    const double hiPreDb  = get (in, "hiShelfGain-pre", 0.0);
    const double loPostHz = get (in, "loShelfFreq-post", 1.0);
    const double loPostDb = get (in, "loShelfGain-post", 0.0);
    const double hiPostHz = get (in, "hiShelfFreq-post", 1.0);
    const double hiPostDb = get (in, "hiShelfGain-post", 0.0);

    // P2: dbtoa runs before the codebox, so the prototype's realised shelf gain
    // is 10^(inlet/20) dB. Converting is the default because it puts the SAME
    // shelf on both sides and leaves P1 - the alpha bug, which nothing can undo
    // from the Max end - as the only shelf difference left to measure.
    auto shelfDb = [&opt] (double inletDb)
    {
        return opt.shelfGainNominal ? inletDb : protoShelfDb (inletDb);
    };

    d.preLoShelfHz  = f (clampRep (r, "effectDistPreLoShelfFreq",  loPreHz,  20.0,   2000.0));
    d.preLoShelfDb  = f (clampRep (r, "effectDistPreLoShelfGain",  shelfDb (loPreDb), -24.0,  24.0));
    d.preHiShelfHz  = f (clampRep (r, "effectDistPreHiShelfFreq",  hiPreHz, 1000.0, 20000.0));
    d.preHiShelfDb  = f (clampRep (r, "effectDistPreHiShelfGain",  shelfDb (hiPreDb), -24.0,  24.0));
    d.postLoShelfHz = f (clampRep (r, "effectDistPostLoShelfFreq", loPostHz, 20.0,   2000.0));
    d.postLoShelfDb = f (clampRep (r, "effectDistPostLoShelfGain", shelfDb (loPostDb), -24.0, 24.0));
    d.postHiShelfHz = f (clampRep (r, "effectDistPostHiShelfFreq", hiPostHz, 1000.0, 20000.0));
    d.postHiShelfDb = f (clampRep (r, "effectDistPostHiShelfGain", shelfDb (hiPostDb), -24.0, 24.0));

    rec (r, "dist.driveDb", d.driveDb);
    rec (r, "dist.shape (= 1 - waveform/100, INVERTED)", d.shape);
    rec (r, "dist.bias", d.bias);
    rec (r, "dist.outputDb", d.outputDb);
    rec (r, "dist.mix (= 100 - dryWet)", d.mix);
    rec (r, "dist.oversample (1 = off)", d.oversample);
    const char* shelfNote = opt.shelfGainNominal
        ? "dist shelf gains: NOMINAL, sent straight through (P2 left in)"
        : "dist shelf gains: converted by 10^(inlet/20) dB, the prototype's realised gain (P2 taken out)";
    r.native.emplace_back (shelfNote);

    rec (r, "dist.preLoShelfHz", d.preLoShelfHz);   rec (r, "dist.preLoShelfDb", d.preLoShelfDb);
    rec (r, "dist.preHiShelfHz", d.preHiShelfHz);   rec (r, "dist.preHiShelfDb", d.preHiShelfDb);
    rec (r, "dist.postLoShelfHz", d.postLoShelfHz); rec (r, "dist.postLoShelfDb", d.postLoShelfDb);
    rec (r, "dist.postHiShelfHz", d.postHiShelfHz); rec (r, "dist.postHiShelfDb", d.postHiShelfDb);

    // Below -40 dB on the inlet the prototype's shelf is the identity to within
    // 0.01 dB, so neither defect can show and the corner stops mattering too.
    const bool neutral = shelfIsNeutral (loPreDb) && shelfIsNeutral (hiPreDb)
                      && shelfIsNeutral (loPostDb) && shelfIsNeutral (hiPostDb);
    if (! neutral)
    {
        r.active.push_back (&kP1);
        if (opt.shelfGainNominal)
            r.active.push_back (&kP2);
    }
}

inline void convertDynamics (const Values& in, double, const Options& opt, MapResult& r)
{
    using namespace detail;
    auto& d = r.params.dyn[0];
    d.bypass = 0;
    d.compOn = 1;
    d.expOn = 1;                    // the prototype always runs both stages
    d.detector = 0;                 // peak, as the prototype's abs/atodb chain
    d.autoMakeup = 0;
    d.lookaheadMs = 0.0f;           // the prototype delays no audio
    d.compKneeDb = 0.0f;            // hard knee on both sides
    d.expHoldMs = 0.0f;             // the prototype has no hold
    d.expRangeDb = -80.0f;          // the widest we offer; the prototype has no range

    const double cThr  = get (in, "Cthreshold", 0.0);
    const double cRat  = get (in, "Cratio", 1.0);
    const double cAtk  = get (in, "Cattack", 10.0);
    const double cRel  = get (in, "Crelease", 100.0);
    const double cLo   = get (in, "CscLoFreq", 20.0);
    const double cHi   = get (in, "CscHiFreq", 20.0);
    const double cDel  = get (in, "CdetectionDelay", 0.0);
    const double xThr  = get (in, "Xthreshold", 0.0);
    const double xRat  = get (in, "Xratio", 1.0);
    const double xAtk  = get (in, "Xattack", 10.0);
    const double xRel  = get (in, "Xrelease", 10.0);
    const double xLo   = get (in, "XscLoFreq", 20.0);
    const double xHi   = get (in, "XscHiFreq", 20.0);
    const double gain  = get (in, "gain", 0.0);

    const double atkSource = opt.dynMatchProtoTimeWiring ? cRel : cAtk;
    const double relSource = opt.dynMatchProtoTimeWiring ? cAtk : cRel;

    d.compThresholdDb     = f (clampRep (r, "effectDynCompThreshold", cThr, -60.0, 0.0));
    d.compRatio           = f (clampRep (r, "effectDynCompRatio", cRat, 1.0, 100.0));
    d.compAttackMs        = f (clampRep (r, "effectDynCompAttack", atkSource, 0.05, 200.0));
    d.compReleaseMs       = f (clampRep (r, "effectDynCompRelease", relSource, 5.0, 2000.0));
    d.compScLoCutHz       = f (clampRep (r, "effectDynCompScLoCut", cLo, 20.0, 2000.0));
    d.compScHiCutHz       = f (clampRep (r, "effectDynCompScHiCut", cHi, 1000.0, 20000.0));
    d.compDetectorDelayMs = f (clampRep (r, "effectDynCompDetectorDelay", cDel, 0.0, 50.0));

    d.expThresholdDb = f (clampRep (r, "effectDynExpThreshold", xThr, -90.0, 0.0));
    d.expRatio       = f (clampRep (r, "effectDynExpRatio", xRat, 1.0, 100.0));
    d.expAttackMs    = f (clampRep (r, "effectDynExpAttack", xAtk, 0.05, 200.0));
    d.expReleaseMs   = f (clampRep (r, "effectDynExpRelease", xRel, 5.0, 2000.0));
    d.expScLoCutHz   = f (clampRep (r, "effectDynExpScLoCut", xLo, 20.0, 2000.0));
    d.expScHiCutHz   = f (clampRep (r, "effectDynExpScHiCut", xHi, 1000.0, 20000.0));
    d.makeupDb       = f (clampRep (r, "effectDynMakeup", gain, -24.0, 24.0));

    rec (r, "dyn[0].compThresholdDb", d.compThresholdDb);
    rec (r, "dyn[0].compRatio", d.compRatio);
    rec (r, opt.dynMatchProtoTimeWiring ? "dyn[0].compAttackMs (= Crelease, proto WIRING)"
                                        : "dyn[0].compAttackMs (= Cattack, proto INTENT)",
         d.compAttackMs);
    rec (r, opt.dynMatchProtoTimeWiring ? "dyn[0].compReleaseMs (= Cattack, proto WIRING)"
                                        : "dyn[0].compReleaseMs (= Crelease, proto INTENT)",
         d.compReleaseMs);
    rec (r, "dyn[0].compScLoCutHz", d.compScLoCutHz);
    rec (r, "dyn[0].compScHiCutHz", d.compScHiCutHz);
    rec (r, "dyn[0].compDetectorDelayMs", d.compDetectorDelayMs);
    rec (r, "dyn[0].expThresholdDb", d.expThresholdDb);
    rec (r, "dyn[0].expRatio", d.expRatio);
    rec (r, "dyn[0].expAttackMs", d.expAttackMs);
    rec (r, "dyn[0].expReleaseMs", d.expReleaseMs);
    rec (r, "dyn[0].expScLoCutHz", d.expScLoCutHz);
    rec (r, "dyn[0].expScHiCutHz", d.expScHiCutHz);
    rec (r, "dyn[0].makeupDb", d.makeupDb);

    r.active.push_back (&kDynSlide);
    r.active.push_back (&kDynStartup);

    if (xRat != 1.0)
        r.active.push_back (&kP3);

    // The prototype's own boxes default both sidechain high cuts to 20 Hz, and
    // our biquad clamps a request below 1000 Hz back up, so anything under the
    // floor means the two detectors are looking at different signals.
    if (cHi < 1000.0 || xHi < 1000.0)
        r.active.push_back (&kP4);

    if (! opt.dynMatchProtoTimeWiring && cAtk != cRel)
        r.active.push_back (&kDynTimes);

    if (cDel > 0.0)
        r.active.push_back (&kDynDetDelay);
}

inline void convertChorus (const Values& in, double sr, const Options& opt, MapResult& r)
{
    using namespace detail;
    auto& m = r.params.mod;
    m.bypass = 0;
    m.mode = 0;                     // chorus
    m.voices = 1;                   // the prototype's line 1
    m.shape = static_cast<std::uint8_t> (spatcore::dsp::LFOWaveforms::Sine);
    m.throughZero = 0;

    const double d1     = get (in, "delaytime1", 0.0);
    const double d2on   = get (in, "delaytime2onOff", 0.0);
    const double speed  = get (in, "LFOspeed", 0.0);
    const double amount = get (in, "LFOamount", 0.0);
    const double pol    = get (in, "feedbackPolarity", 0.0);
    const double loCut  = get (in, "loCutFreq", 20.0);
    const double drywet = get (in, "drywet", 0.0);

    // gen~ decodes the polarity as (!= 0) for the enable and (== 2) for the
    // sign, so the coefficient is exactly 0, +1 or -1 - there is no amount.
    const double fbPct = (pol == 0.0) ? 0.0 : (pol == 2.0 ? -100.0 : 100.0);

    m.delayMs  = f (clampRep (r, "effectModDelay", d1 / sr * 1000.0, 0.1, 50.0));
    m.rateHz   = f (clampRep (r, "effectModRate", speed, 0.05, 10.0));
    m.depth    = f (clampRep (r, "effectModDepth", amount, 0.0, 100.0));
    m.feedback = f (clampRep (r, "effectModFeedback", fbPct, -95.0, 95.0));
    m.phaseDeg = 180.0f;            // our Sine is -cos; the prototype's `cycle` is +cos
    m.loCutHz  = f (clampRep (r, "effectModLoCut", loCut, 20.0, 2000.0));
    m.mix      = f (clampRep (r, "effectModMix", 100.0 - drywet, 0.0, 100.0));

    rec (r, "mod.mode (0 = chorus)", m.mode);
    rec (r, "mod.voices", m.voices);
    rec (r, "mod.delayMs (= delaytime1 samples / rate)", m.delayMs);
    rec (r, "mod.rateHz", m.rateHz);
    rec (r, "mod.depth", m.depth);
    rec (r, "mod.feedback (from the 3-state polarity)", m.feedback);
    rec (r, "mod.phaseDeg (180 aligns our -cos with `cycle`)", m.phaseDeg);
    rec (r, "mod.loCutHz", m.loCutHz);
    rec (r, "mod.mix (= 100 - drywet)", m.mix);

    r.active.push_back (&kModSamples);
    r.active.push_back (&kModMode);

    if (pol != 0.0)
    {
        r.active.push_back (&kChorusFeedback);
        r.active.push_back (&kModFbFilter);
    }

    if (d2on != 0.0)
    {
        r.active.push_back (&kChorusLine2);
        if (! opt.allowUnmapped)
            r.blockers.emplace_back (
                "delaytime2onOff = " + std::to_string (d2on) + " switches on the prototype's "
                "SECOND delay line, which ModulationModule has no counterpart for. Send 0 in "
                "Max, or set \"options\": { \"allowUnmapped\": true } if you understand that "
                "the comparison then measures nothing.");
    }
}

inline void convertFlanger (const Values& in, double sr, const Options&, MapResult& r)
{
    using namespace detail;
    auto& m = r.params.mod;
    m.bypass = 0;
    m.mode = 1;                     // flanger
    m.voices = 1;
    m.shape = static_cast<std::uint8_t> (spatcore::dsp::LFOWaveforms::Sine);
    m.throughZero = 0;

    const double dt     = get (in, "delaytime", 0.0);
    const double fb     = get (in, "feedback", 0.0);
    const double pol    = get (in, "feedbackPolarity", 0.0);
    const double amount = get (in, "LFOamount", 0.0);
    const double speed  = get (in, "LFOspeed", 0.0);
    const double phase  = get (in, "LFOphase", 0.0);
    const double loCut  = get (in, "loCutFreq", 20.0);
    const double drywet = get (in, "drywet", 0.0);

    m.delayMs  = f (clampRep (r, "effectModDelay", dt / sr * 1000.0, 0.1, 50.0));
    m.rateHz   = f (clampRep (r, "effectModRate", speed, 0.05, 10.0));
    m.depth    = f (clampRep (r, "effectModDepth", amount, 0.0, 100.0));
    m.feedback = f (clampRep (r, "effectModFeedback", fb * pol, -95.0, 95.0));
    m.phaseDeg = f (wrap360 (phase + 90.0));
    m.loCutHz  = f (clampRep (r, "effectModLoCut", loCut, 20.0, 2000.0));
    m.mix      = f (clampRep (r, "effectModMix", 100.0 - drywet, 0.0, 100.0));

    rec (r, "mod.mode (1 = flanger)", m.mode);
    rec (r, "mod.voices", m.voices);
    rec (r, "mod.delayMs (= delaytime samples / rate)", m.delayMs);
    rec (r, "mod.rateHz", m.rateHz);
    rec (r, "mod.depth", m.depth);
    rec (r, "mod.feedback (= feedback * feedbackPolarity)", m.feedback);
    rec (r, "mod.phaseDeg (= LFOphase + 90)", m.phaseDeg);
    rec (r, "mod.loCutHz", m.loCutHz);
    rec (r, "mod.mix (= 100 - drywet)", m.mix);

    r.active.push_back (&kModSamples);
    r.active.push_back (&kModMode);

    if (fb * pol != 0.0)
        r.active.push_back (&kModFbFilter);
}

inline void convertDelay (const Values& in, double, const Options& opt, MapResult& r)
{
    using namespace detail;
    auto& d = r.params.delay;
    d.bypass = 0;
    d.taps = 1;                     // the prototype is a single tap with feedback
    d.tapMode = 0;                  // manual, so tapTimeMs[0] is the tap
    d.pattern = 0;
    d.feedbackTap = 0;              // 0 = the last active tap = tap 1
    d.diffusion = 0.0f;             // no counterpart in the prototype
    d.glideMs = 200.0f;             // nothing moves in a static A/B

    const double secs   = get (in, "delayTime", 0.0);
    const double modHz  = get (in, "modFreq", 0.1);
    const double modD   = get (in, "modDepth", 0.0);
    const double loCut  = get (in, "loCutFreq", 20.0);
    const double loSHz  = get (in, "loShelfFreq", 1.0);
    const double loSDb  = get (in, "loShelfGain", 0.0);
    const double hiSHz  = get (in, "hiShelfFreq", 1.0);
    const double hiSDb  = get (in, "hiShelfGain", 0.0);
    const double fb     = get (in, "feedback", 0.0);
    const double drywet = get (in, "dryWet", 0.0);

    // `* 1000.` then `max 1` then mstosamps: the floor is 1 ms, so a delayTime
    // of 0 is a 1 ms delay and not silence.
    const double ms = std::max (1.0, secs * 1000.0);

    d.timeMs = f (clampRep (r, "effectDelayTime", ms, 1.0, 5000.0));
    for (int k = 0; k < 8; ++k)
    {
        d.tapTimeMs[k] = d.timeMs;
        d.tapLevelDb[k] = 0.0f;     // the prototype's line is written at unity
    }

    d.modRateHz   = f (clampRep (r, "effectDelayModRate", modHz, 0.02, 10.0));
    d.modDepthPct = f (clampRep (r, "effectDelayModDepth", modD * 100.0, 0.0, 50.0));
    d.inLoCutHz   = f (clampRep (r, "effectDelayInLoCut", loCut, 20.0, 2000.0));
    d.fbLoShelfHz = f (clampRep (r, "effectDelayFbLoShelfFreq", loSHz, 20.0, 2000.0));
    d.fbLoShelfDb = f (clampRep (r, "effectDelayFbLoShelfGain",
                                 opt.shelfGainNominal ? loSDb : protoShelfDb (loSDb), -24.0, 24.0));
    d.fbHiShelfHz = f (clampRep (r, "effectDelayFbHiShelfFreq", hiSHz, 1000.0, 20000.0));
    d.fbHiShelfDb = f (clampRep (r, "effectDelayFbHiShelfGain",
                                 opt.shelfGainNominal ? hiSDb : protoShelfDb (hiSDb), -24.0, 24.0));
    d.feedback    = f (clampRep (r, "effectDelayFeedback", fb, 0.0, 95.0));
    d.mix         = f (clampRep (r, "effectDelayMix", 100.0 - drywet, 0.0, 100.0));

    rec (r, "delay.taps", d.taps);
    rec (r, "delay.tapMode (0 = manual)", d.tapMode);
    rec (r, "delay.timeMs (= max(1, delayTime seconds * 1000))", d.timeMs);
    rec (r, "delay.tapTimeMs[0]", d.tapTimeMs[0]);
    rec (r, "delay.tapLevelDb[0]", d.tapLevelDb[0]);
    rec (r, "delay.modRateHz", d.modRateHz);
    rec (r, "delay.modDepthPct (= modDepth * 100)", d.modDepthPct);
    rec (r, "delay.inLoCutHz", d.inLoCutHz);
    r.native.emplace_back (opt.shelfGainNominal
        ? "delay shelf gains: NOMINAL, sent straight through (P2 left in)"
        : "delay shelf gains: converted by 10^(inlet/20) dB, the prototype's realised gain (P2 taken out)");
    rec (r, "delay.fbLoShelfHz", d.fbLoShelfHz);
    rec (r, "delay.fbLoShelfDb", d.fbLoShelfDb);
    rec (r, "delay.fbHiShelfHz", d.fbHiShelfHz);
    rec (r, "delay.fbHiShelfDb", d.fbHiShelfDb);
    rec (r, "delay.feedback", d.feedback);
    rec (r, "delay.diffusion", d.diffusion);
    rec (r, "delay.mix (= 100 - dryWet)", d.mix);

    r.active.push_back (&kDelayFirstRepeat);

    if (! (shelfIsNeutral (loSDb) && shelfIsNeutral (hiSDb)))
    {
        r.active.push_back (&kP1);
        if (opt.shelfGainNominal)
        {
            r.active.push_back (&kP2);
            r.active.push_back (&kDelayShelves);
        }
        if (loSHz < 20.0 || hiSHz < 1000.0)
            r.active.push_back (&kDelayShelfRange);
    }

    if (modD > 0.0)
        r.active.push_back (&kDelayModPhase);
}

//==============================================================================
struct Patch
{
    const char* name;               ///< the .gendsp file, without the extension
    const char* module;             ///< our chain-order token
    ModuleId id;
    bool nonlinear;                 ///< picks the -30 dB residual target
    const char* summary;
    const Inlet* inlets;
    int numInlets;
    void (*convert) (const Values&, double, const Options&, MapResult&);
};

inline const Patch kPatches[] =
{
    { "fx_tremolo", "trem", ModuleId::Trem, false,
      "Amplitude modulation, dB-linear depth, sine/triangle blend.",
      kTremoloInlets, 4, &convertTremolo },

    { "fx_bitcrusher", "crush", ModuleId::Crush, true,
      "Quantiser plus dither. No sample-rate reduction in the prototype.",
      kBitcrusherInlets, 3, &convertBitcrusher },

    { "fx_distortion", "dist", ModuleId::Dist, true,
      "Pre shelves, drive, clip/tanh blend, post shelves, output, dry/wet.",
      kDistortionInlets, 12, &convertDistortion },

    { "fx_dynamics", "dyn1", ModuleId::Dyn, true,
      "Compressor then expander, parallel detectors off the module input.",
      kDynamicsInlets, 14, &convertDynamics },

    { "fx_chorus", "mod", ModuleId::Mod, false,
      "Two modulated delay lines with a 3-state polarity feedback (line 2 unmapped).",
      kChorusInlets, 9, &convertChorus },

    { "fx_flanger", "mod", ModuleId::Mod, false,
      "One modulated delay line, signed feedback, LFO phase in degrees.",
      kFlangerInlets, 8, &convertFlanger },

    { "fx_delay", "delay", ModuleId::Delay, false,
      "Single tap with feedback; two shelves inside the loop, input cut outside it.",
      kDelayInlets, 10, &convertDelay },
};

inline constexpr int kNumPatches = 7;
static_assert (sizeof (kPatches) / sizeof (kPatches[0]) == kNumPatches,
               "kNumPatches must track kPatches");

inline const Patch* findPatch (const std::string& name)
{
    for (const auto& p : kPatches)
        if (name == p.name)
            return &p;

    // Accept the file name too, so "fx_delay.gendsp" works.
    for (const auto& p : kPatches)
        if (name == std::string (p.name) + ".gendsp")
            return &p;

    return nullptr;
}

inline const Inlet* findInlet (const Patch& p, const std::string& name)
{
    for (int i = 0; i < p.numInlets; ++i)
        if (name == p.inlets[i].name)
            return &p.inlets[i];

    return nullptr;
}

/** Resolve a caller's inlet values against the patch's declared defaults, then
    convert. Unknown inlet names are BLOCKERS, not warnings: a misspelt control
    that silently took its default is exactly the failure this file exists to
    prevent. */
inline MapResult map (const Patch& p, const Values& given, double sampleRate, const Options& opt)
{
    MapResult r;
    r.module = p.module;
    r.moduleId = p.id;
    r.nonlinear = p.nonlinear;

    for (const auto& kv : given)
        if (findInlet (p, kv.first) == nullptr)
            r.blockers.emplace_back ("'" + kv.first + "' is not an inlet of " + p.name
                                     + " - run with --print-mapping " + p.name
                                     + " for the list");

    Values resolved;
    for (int i = 0; i < p.numInlets; ++i)
    {
        const Inlet& in = p.inlets[i];
        const auto it = given.find (in.name);
        const bool present = it != given.end();
        const double v = present ? it->second : in.defaultValue;
        resolved[in.name] = v;

        char buf[192];
        std::snprintf (buf, sizeof (buf), "inlet %-2d %-18s = %-12.6g %-8s (%s)",
                       in.index, in.name, v, in.unit,
                       present ? "given" : "gen~ default");
        r.resolved.emplace_back (buf);
    }

    if (r.blockers.empty())
        p.convert (resolved, sampleRate, opt, r);

    return r;
}

} // namespace abmap
