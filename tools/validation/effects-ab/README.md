# effects-ab — does the port SOUND like the Max prototype?

This is the Max/MSP A/B harness of §10 of `Documentation/effects-channels-plan.md`.
It answers the one question no unit test can: *does our ported effect module sound like
the `gen~` patch it was written from?*

It is **not a CI gate**. §10 says so, and you have said the prototypes are starting
points open to improvement rather than bit-for-bit targets. Nothing here runs on a
push, nothing here blocks a merge. It runs when you want to hear whether a port landed.

---

## The shape of it

```
   gen_input.py  ─────────────►  ab_input.wav  ─────────────►  Max (you render)
                                      │                              │
                                      │                              ▼
                                      ▼                        max_out.wav
                             ab-render (C++, this dir)               │
                                      │                              │
                                      ▼                              │
                          ours.wav + ours.map.json  ◄────────────────┘
                                      │
                                      ▼
                                ab_compare.py
                       RMS residual · 1/3-octave · LFO rate & depth
```

`ab_input.wav` is the same ten seconds every time, on every platform. Both sides render
*that* file, the comparator aligns the two and reports three metrics against the §10
targets — and, crucially, tells you which of the differences it found are **supposed to
be there**.

---

## Requirements

**spatcore must be at `a4e4e57` or a descendant.** `ab_render.cpp` includes
`spatcore/effects/EffectChain.h`, and the `effects/` tree only reaches spatcore's main
line at `a4e4e57` (PR #12, *feature/effects-phase2*). Check with:

```
git -C spatcore log --oneline -1
git -C spatcore ls-tree --name-only HEAD effects/       # must print something
```

If `effects/` is missing, run `git submodule update --init spatcore`. CMake checks this
at configure time and stops with the same instructions rather than letting you walk into
a compiler error on the first `#include`.

> **If you are the one committing this directory:** commit it *together with the
> spatcore submodule bump*. An adversarial review found the superproject recording
> `ed60a80` (no `effects/` at all) while the working tree happened to have `a4e4e57`
> checked out - so the harness built locally and could not have been built from a
> fresh clone. `git diff spatcore` must be empty when you are done.

Python 3 with numpy, for the three scripts. No Max licence is needed for anything in
this directory; Max is needed only to produce the `max_out.wav` you compare against.

---

## Before you start (once)

> **Where to run things.** The two `cmake` lines below are run from the **repo root**.
> Everything after that is run from **this directory** (`tools/validation/effects-ab`),
> which is where `gen_input.py` writes `ab_input.wav` and where `params/` lives. Below,
> `ab-render` is shorthand for `build\ab-render_artefacts\Release\ab-render.exe`.

**1. Build the renderer.**

There is **no `cmake` on PATH** on the dev box. Either open a *Developer PowerShell for
VS 2026* (which puts one there), or call the bundled one by its full path:

```
$cmake = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake -S tools/validation/effects-ab -B tools/validation/effects-ab/build -G "Visual Studio 18 2026"
& $cmake --build tools/validation/effects-ab/build --config Release
```

The executable lands at `build/ab-render_artefacts/Release/ab-render.exe`.
(On macOS/Linux use your usual generator; it links four JUCE modules and one `.cpp`.)

**2. Check the harness itself still works.**

```
python ab_selftest.py
```

Sixty checks: the input is deterministic, all seven patches render reproducibly, a render
compared against itself nulls to exactly zero, a known level offset / latency / noise
level reads back as exactly that, a known LFO error is measured as that error, and the
expected-difference machinery both excuses and (under `--strict`) stops excusing. If this
fails, fix it before believing anything the harness tells you about Max.

**What the self-test does not do.** Every comparison in it is a render against itself
or against a synthetic alteration of itself, so it checks the *harness*, not the
*modules*. Verified: a deliberate 10% feedback error injected into
`MultitapDelayModule.h`, and a 50% depth error injected into `TremoloModule.h`, both
leave `ab_selftest.py` printing `all 60 checks passed`. Nothing in this repository
tests whether a module matches its Max prototype. Only a `max_out.wav` you made
yourself can do that - which is the whole point of the tool.

**3. Know what the mapping does to your numbers.**

```
ab-render --list
ab-render --print-mapping fx_delay        (or  --print-mapping all)
```

This prints every inlet of that patch, its `gen~` default, its unit, and exactly how it
converts. Read it once per patch before you type anything into Max. The conventions are
**not** the same on both sides, and the differences are silent:

| prototype says | we say | conversion |
|---|---|---|
| `dryWet` / `drywet` (a **dry** per cent, through `wetDry.gendsp`) | `effect*Mix`, a **wet** per cent | `mix = 100 − inlet` |
| `fx_tremolo`'s `drywet` — the exception, its own arithmetic | wet per cent | `mix = inlet × 100` (it is a **fraction**, and it is **wet**) |
| `fx_distortion`'s `waveform`, a per cent of the **hard clipper** | `effectDistShape`, 0 = clip … 1 = tanh | `shape = 1 − inlet/100` — **inverted** |
| `fx_delay`'s `delayTime`, in **seconds**, floored at 1 ms | `effectDelayTime`, in **ms** | `ms = max(1, s × 1000)` |
| `fx_chorus` / `fx_flanger` delay times, in **samples** | `effectModDelay`, in **ms** | `ms = samples / rate × 1000` — **rate dependent** |
| `fx_delay`'s `modDepth`, a raw **fraction** | `effectDelayModDepth`, a per cent | `× 100` |
| `fx_flanger`'s `feedback` × `feedbackPolarity` | one **signed** per cent | `feedback × polarity` |
| `fx_chorus`'s `feedbackPolarity`, a 3-state enum with no amount | signed per cent | `0 / −100 / +100`, then clamped to ±95 |
| `fx_flanger`'s `LFOphase`, in degrees | `effectModPhase` | `+ 90` (the prototype's LFO is `sin`, ours is `−cos`) |
| shelf **gain** inlets | `effect*ShelfGain` in dB | `10^(inlet/20)` — see P2 below |

You never apply these by hand. You write down the numbers **exactly as you typed them
into Max**, and `ab_mapping.h` converts. An inlet name it does not recognise is a hard
refusal, not a default — a typo cannot silently become a comparison of two different
settings.

---

## The procedure, in order

### Step 1 — make the input file

```
python gen_input.py
```

Writes `ab_input.wav`: 10 s, mono, 48 kHz, 32-bit float, and prints a SHA-256. **Keep
that hash.** Both renders must come from this exact file, and both tools check.

What is in it, and why:

| time | content | what it exercises |
|---|---|---|
| 0.00–0.20 | silence | lead-in; the alignment's zero reference |
| 0.20 | one-sample impulse at +0.9 | impulse response, delay times, latency |
| 0.60–1.00 | DC step at +0.5 and its release | DC blockers, envelope followers |
| 1.20–3.60 | eight tones, 55 Hz → 7040 Hz, 0.3 s each | per-band behaviour |
| **3.60–5.60** | **steady 1 kHz** | **the LFO window** — rate and depth are measured here |
| 5.60–6.60 | log sweep 20 Hz → 20 kHz | shelves, cuts, phaser notches |
| 6.60–8.00 | four decaying 1 kHz bursts, 25 dB apart | compressor/expander envelopes |
| 8.00–8.60 | 220 + 3150 Hz together | intermodulation from nonlinearities |
| 8.60–9.20 | full-band noise | broadband spectrum |
| 9.20–9.60 | noise at −48 dBFS, then at −6 dBFS | gates, expanders, dynamics |
| 9.60–10.00 | silence | tails ring out; something to align on |

`python gen_input.py --print-plan` prints this table.

### Step 2 — render it in Max

Set up a **non-realtime, sample-exact** render. The details matter more than the object
names, so here are the requirements first and one concrete recipe after.

**Requirements**

1. **48 kHz.** Audio Status → Sample Rate 48000. `fx_chorus`, `fx_flanger` and
   `fx_bitcrusher` all behave differently at another rate, and the chorus/flanger delay
   times are literally in samples.
2. **NonRealTime driver.** Audio Status → Driver → *NonRealTime*. A realtime render that
   drops one vector is a render you cannot trust, and you will not hear it drop.
3. **Sample-exact playback from sample 0.** `sfplay~` starts on a message, which lands
   somewhere inside a signal vector. Use `buffer~` + `count~` + `index~` instead.
4. **Nothing else in the chain.** The `gen~` output goes straight to the recorder — no
   master fader, no limiter, no dither, no `clip~`.
5. **Record the whole 10 s** (a little more is fine — the comparator trims).
6. **Export WAV, 32-bit float** if your `sfrecord~` offers it; 24-bit integer is fine
   (its floor sits 144 dB down, far below any target here). **Not 16-bit**: its floor is
   only 96 dB down and it clips anything over full scale.

**One recipe**

```
[buffer~ abin]                     ← message: replace ab_input.wav
[count~]  →  [index~ abin]         ← sample-exact, starts at sample 0
             │
             ▼
[gen~ @gen fx_delay.gendsp]        ← the prototype, inlets driven by number boxes
             │
             ▼
[sfrecord~ 1]                      ← messages: open max_out.wav  /  1 … 0
```

Turn DSP on, send `0` to `count~` to park it at sample 0, start `sfrecord~`, then release
`count~`. If the recorder starts a vector early or late it does not matter: the
comparator finds and removes a lag of up to ±250 ms, and prints what it found.

**Before you render, look at the "in Max" lines** that `ab-render --print-mapping <patch>`
printed. (Those lines are printed by `--print-mapping` for the gen~ defaults and again
by the render for the set your `params.json` actually activates - an earlier version
printed them only at render time, i.e. after the Max pass they were meant to inform.) Several of them tell you a number to send that removes a prototype defect from
the comparison — `CscHiFreq 20000` on the dynamics, `−200` on shelf gains, `0` on the
chorus feedback polarity. Setting those turns a meaningless A/B into a useful one.

### Step 3 — write down what you typed

Copy one of the files in `params/` and edit the `"prototype"` block so that every value
is the number **you actually sent to that inlet in Max**. Leave an inlet out and it takes
the `gen~` default, which is printed back to you so you can check.

```json
{
  "patch": "fx_delay",
  "options": { "shelfGainNominal": false },
  "prototype": {
    "delayTime": 0.375,
    "feedback": 40.0,
    "dryWet": 65.0,
    "loShelfGain": -200.0,
    "hiShelfGain": -200.0
  }
}
```

### Step 4 — render our side

```
ab-render --params params/delay.json --in ab_input.wav --out out/delay.ours.wav \
          --expect-sha <the hash gen_input.py printed>
```

It prints the resolved inlets, the parameters it set on our module, any clamp it had to
apply, and the expected differences now in force. It writes `out/delay.ours.wav` and
`out/delay.ours.map.json`, which is what tells the comparator about the defects.

### Step 5 — compare

```
python ab_compare.py --max max_out.wav --ours out/delay.ours.wav --input ab_input.wav
```

---

## What a good result looks like

| metric | target | meaning |
|---|---|---|
| RMS residual | **≤ −40 dB** linear, **≤ −30 dB** nonlinear | the whole-file difference, relative to the Max render |
| 1/3-octave spectral | the same −40 / −30 dB | the worst band error, converted to an equivalent residual so one target serves both. −40 dB is 0.087 dB of band error; −30 dB is 0.274 dB |
| LFO rate | **≤ 1 %** | measured from the envelope of the steady 1 kHz probe |
| LFO depth | **≤ 0.5 dB** | the same envelope, 99th vs 1st percentile |

The two LFO metrics measure whatever periodicity the probe's envelope has — an actual
LFO, but also a delay's repeats beating against the tone. That is fine, because both
sides are measured the same way, but do not read "LFO rate 0.451 Hz" on a delay render
as a claim that the delay has an LFO. When neither side shows a peak at least 6 dB above
the envelope's noise floor the metrics report `n/a` rather than guessing.

Linear modules: tremolo, chorus, flanger, delay (and phaser, EQ, reverb, which have no
prototype). Nonlinear: distortion, dynamics, bitcrusher. `ab-render` picks the right
target and writes it into the sidecar; the comparator reads it.

The comparator also prints a **level match** line. That is not a metric — it is a
diagnosis. A residual of −0.04 dB with a level match of +6.000 dB means the two renders
are the same shape at different gains, which is almost always a dry/wet or an output-gain
conversion gone wrong, not a DSP difference.

**A residual is only meaningful when the level match is near 0 dB and the lag is small
and stable.** If the lag comes back at the ±search limit, raise `--max-lag-ms`; if it
comes back at some large arbitrary number, the two renders are not of the same signal.

### What −40 dB actually buys you

Measured on `fx_delay` with `params/delay.json`, by rendering a known feedback error and
comparing it against the pristine render as if it were `max_out.wav`:

| feedback error | RMS residual | verdict |
|---|---|---|
| +2% (40.0 → 40.8) | −45.33 dB | PASS, exit 0 |
| +10% (40.0 → 44.0) | −31.01 dB | 3 metrics FAIL, exit 1 |

So on the flagship module the −40 dB target corresponds to roughly a **4% parameter
error**. Below that this harness will not see it; a −40 dB pass is not a claim of
bit-equality.

Reproduce it yourself in four lines - no module source is edited, so nothing in
`spatcore/` is disturbed:

```
python -c "import json; d=json.load(open('params/delay.json'));
           d['prototype']['feedback']=40.8;
           json.dump(d,open('fb.json','w'),indent=2)"
ab-render --params fb.json --in ab_input.wav --out out/fb.wav
python ab_compare.py --max out/delay.ours.wav --ours out/fb.wav ^
       --sidecar out/delay.ours.map.json --input ab_input.wav
```

The +10% row is also the clearest demonstration of the LFO caveat above: it reports
`LFO rate ... FAIL` on a render whose `modDepth` is 0. There is no LFO - that is the
delay's repeats beating against the probe tone, and both sides are measured the same
way, so the number is meaningful even though its *name* is not. Do not go hunting an
LFO on a delay.

### When the comparator refuses to give a verdict

A comparison that measured nothing is not a pass. `ab_compare.py` stops with **exit 4**
rather than printing a verdict when:

- **the Max export is short.** `overlap()` trims to the shorter of the two files, so an
  `sfrecord~` stopped early used to compare its 1 second against our 10 and report
  `all metrics passed`. The compared span is now printed as a percentage of our render
  and anything under 98% is an error. `--allow-short-span` downgrades it to a warning
  plus a CAVEAT line on the result; `--min-span FRAC` moves the bar.
- **either side is silent over the compared span.** Every metric here is a ratio to the
  Max render, so a silent reference made the residual −inf and therefore a PASS. Two
  silent files used to be a clean green run.
- **fewer than two metrics produced a verdict.** Metrics drop out quietly - the
  spectral one returns nothing when no band is usable, the LFO one reports `n/a` - and
  `all metrics passed` was being printed over what was left. Each drop-out now prints
  an `n/a` line saying why, and too few survivors reads `RESULT: INCONCLUSIVE`.

Other exit codes: **1** a metric failed, **2** bad arguments or unreadable input,
**3** the `--input` fingerprint does not match the one our render started from.

---

## The differences that are SUPPOSED to be there

This is the part that saves you a week. §5.13 of the plan records **four defects in the
prototypes** that our modules deliberately do not reproduce. On the parameters they touch
**a large difference is the correct answer.** A harness that flagged them as failures
would be telling you to break working code, so `ab_compare.py` reports the metrics they
affect as `EXPECTED-DIFF` instead of `FAIL`, names the defect, and tells you what (if
anything) you can set in Max to take it out of the comparison.

| id | what the prototype does | what you can do in Max |
|---|---|---|
| **P1** | `loShelf`/`hiShelf` compute `alpha = sin(ω/2)·sqrt((A + 1/A)/S + 1)` — RBJ is `sqrt((A + 1/A)(1/S − 1) + 2)`. At a nominal S = 0.7 the prototype really runs S ≈ 0.52. Measured here against the corrected law: **0.06 dB** at a 1 dB shelf, 0.19 dB at 3 dB, 0.39 dB at 6 dB, 0.86 dB at 12 dB, **2.07 dB at 24 dB**. | Send **−200** to every shelf **gain** inlet. The prototype's `A` collapses to 1 and an RBJ shelf at A = 1 is the identity whatever its alpha; the mapping sets our own shelf to 0 dB, which the module switches off. **A nominal 0 dB does not do this on either side.** |
| **P2** | The shelf gain goes through `dbtoa` *before* a codebox that treats it as dB, so the realised shelf gain is `10^(inlet/20)` dB: a nominal **0 dB shelf really applies +1.00 dB**, 6 dB applies +1.995 dB, and a nominal +40 dB asks for +100 dB. | Nothing — but you do not need to. The mapping **converts by default**, so both sides run the same shelf and only P1 is left. Set `"shelfGainNominal": true` if you want the raw knob-for-knob difference instead. |
| **P3** | `fx_dynamics`'s expander uses the *compressor's* slope, `1 − 1/R`, so it realises `2 − 1/R`: 1.5:1 at a nominal 2:1, saturating at 2:1 however large R gets. It cannot gate. | No inlet value fixes it — the prototype's realisable range stops at 2:1. Send **`Xratio 1`** to take the expander out of the comparison and A/B the compressor alone. |
| **P4** | `hiCut.gendsp`'s second inlet still carries a high *shelf*'s label and `@default 20`, and `fx_dynamics` repeats it: both sidechain high cuts default to **20 Hz**, so the detector sees nothing and the stage barely responds. | **Send `CscHiFreq 20000` and `XscHiFreq 20000`.** This one is entirely removable, and until you do it a dynamics A/B tells you nothing. |

And the further divergences this harness found while writing the mapping — the same
machinery covers them, and `ab-render` lists the ones in force for your settings:

| id | what it is | what you can do |
|---|---|---|
| `D-TREM-WETLEG` | `fx_tremolo`'s wet leg is `in·w·(g − 1)`; that `− 1` is a bug. At full wet the prototype emits `in·(g − 1)` — inverted and near-silent, since `g ≤ 1` always. Ours is `in·((1 − w) + w·g)`. | Nothing, without editing the patch. **Read the LFO rate metric** — it still measures correctly on both sides — and ignore the residual, the spectrum and the depth for this module. |
| `D-CRUSH-DITHER` | Max's `noise` against our `hashNoiseBipolar`: two different streams at the same level never null. | Send **`dither −96`**. The `gen~` `@default 0` is full-scale noise, a placeholder. |
| `D-CRUSH-NORATE` | The prototype is a quantiser and a dither only — there is no sample-and-hold. Our `effectCrushRate` is pinned to the render rate and `effectCrushFilter` to 0. | Nothing to do; the rate half of our module is simply out of scope here. |
| `D-DYN-TIMES` | `Cattack` drives slide-**up** and `Crelease` slide-**down** on a *linear gain that falls when compressing*, so the prototype grabs over `Crelease` and lets go over `Cattack`. Backwards. (The expander beside it uses the same wiring, where it is right by gate convention — which is what makes this a bug and not a house style.) | Set `"options": { "dynMatchProtoTimeWiring": true }` and the mapping follows the wiring instead of the intent, so the two envelopes match. Or type the two numbers swapped in Max. |
| `D-DYN-DETDELAY` | `fx_dynamics`'s detector delay is the only **bare** `delay` in the folder, so it takes `gen~`'s 512-sample default (≈ 10.7 ms at 48 kHz), truncates, does not interpolate, and still delays one sample at a setting of 0. | Send **`CdetectionDelay 0`**. |
| `D-DYN-SLIDE` | `slide` uses `coef = 1/n`; our `EnvelopeFollower` uses `1 − exp(−1/n)`. About 10 % apart at the shortest times, negligible above ~1 ms. | Use attack/release times above 1 ms. |
| `D-DYN-STARTUP` | `slide` histories start at 0 on a linear gain, so the prototype fades in from silence over its attack time. Ours starts settled. | Covered by the input's silent first 0.20 s — as long as the Max render starts at the same sample. |
| `D-CHORUS-FB` | `fx_chorus` has **no feedback amount inlet at all**: the polarity decodes to exactly 0, +1 or −1, so switching it on gives a comb that never decays. Ours clamps at ±95 %. | Send **`feedbackPolarity 0`**. |
| `D-CHORUS-LINE2` | `fx_chorus` runs a *second* independently timed line (entered ÷ 100, gated by a bare multiply that doubles as a level). We have no counterpart — `voices` spreads one line's LFO phase, which is a different thing. | Send **`delaytime2onOff 0`**. With it non-zero the render is **refused** unless you set `"allowUnmapped": true`. |
| `D-MOD-FBFILTER` | Our modulation feedback loop has a DC blocker and a high cut; the prototypes recirculate the raw tap. | Send feedback (or polarity) 0 and the two paths are identical. |
| `D-MOD-SAMPLES` | Chorus and flanger delay times are in **samples**, so the same patch is a different delay at a different rate. | Render both sides at 48 kHz. |
| `D-MOD-MODE` | `effectModMode` is set for the record but `ModulationModule` does not currently read it: chorus and flanger differ only in their settings. | Nothing. |
| `D-DELAY-SHELVES` | Only in `"shelfGainNominal": true` mode: the prototype's two shelves sit *inside* the feedback loop at a 1 Hz corner, so through P2 its loop gains about +1 dB per repeat that ours does not, and the decays diverge further with every repeat. | Leave `shelfGainNominal` off (the default), or send −200 to both shelf gains. |
| `D-DELAY-SHELFRANGE` | `MultitapDelayModule` clamps its feedback shelves to 20–2000 Hz and 1000–20000 Hz; the prototype's 1 Hz defaults cannot be reproduced. | Send a corner inside our range, or neutralise the gains. |
| `D-DELAY-MODPHASE` | `fx_delay` modulates with `cycle` (a **cosine**); our delay's LFO is a sine and — unlike the modulation module — it has no phase control. Two identical modulations 90° apart produce a residual as big as the modulation. | Send **`modDepth 0`** and A/B the delay without modulation. |
| `D-DELAY-FIRSTREPEAT` | The prototype writes its line at unity and scales only the recirculating path, so the first repeat ignores the feedback control. **Ours does the same** — recorded because it is exactly the kind of thing a reader assumes is a bug. | Nothing. It matches. |

When you have removed every removable one from the Max side, run the comparison again
with `--strict`: that stops the excusing and gives you the raw verdict.

> **A note on P2's magnitude.** §5.13 states "+0.5 dB" for a nominal 0 dB shelf. Solving
> the prototype's own coefficients gives **+1.00 dB** — the shelf's asymptotic gain is
> `A²`, and `20·log10(A) = 0.5 dB` is `A` itself expressed in dB. §5.13's other example,
> "a nominal +40 dB asks for +100 dB", agrees with +1.00 dB rather than with +0.5. The
> mapping uses the measured law, `10^(inlet/20)` dB; the numbers are in
> `ab_mapping.h`'s `protoShelfDb`.

---

## A sensible order to work through

1. **`fx_delay`** with the shelves neutralised and `modDepth 0`. It is the cleanest
   comparison available — one tap, one feedback gain, no defect left in play — so it is
   the one that tells you whether your whole rig (rate, alignment, export format) is set
   up correctly. Get this to −40 dB before you believe any other result.
2. **`fx_flanger`**, then **`fx_chorus`** with `feedbackPolarity 0`. Both exercise the
   fractional delay line and the LFO; the flanger also exercises the phase conversion.
3. **`fx_bitcrusher`** with `dither −96`. A pure quantiser, so a residual here is a real
   arithmetic difference.
4. **`fx_distortion`** with all four shelves neutralised. Nonlinear, so the −30 dB target.
   If it fails, try `waveform 0` (pure tanh) and `waveform 100` (pure clip) separately —
   that separates the shaper from the blend.
5. **`fx_dynamics`** with `CscHiFreq 20000`, `XscHiFreq 20000`, `Xratio 1`,
   `CdetectionDelay 0`, and times above 1 ms. Then repeat with
   `"dynMatchProtoTimeWiring": true`.
6. **`fx_tremolo`** last, and only for the LFO rate. Its residual cannot be made small.

`eq1`/`eq2`, `phaser` and `reverb` have no prototype at all. There is nothing to A/B them
against — judge those against §5.2, §5.5 and §5.7 of the plan.

---

## Troubleshooting

| what you see | what it means |
|---|---|
| `the two renders are at different sample rates` | Max's DSP is not at 48 kHz, or `sfrecord~` resampled. |
| lag comes back at the ±search limit | raise `--max-lag-ms`, or the two renders are not of the same signal. |
| `level match ... AND INVERTED` | the Max export is polarity-flipped. Check for a stray `*~ -1`. |
| `RMS residual −0.04 dB, level match +6.000 dB` | the shapes agree, the gains do not: a dry/wet or output-gain conversion, not DSP. |
| residual sits near −20 dB on a delay | a shelf is live on one side. Check the mapping warnings. |
| `n/a (no modulation detected)` | there is no LFO in this render, or `--mod-window` is outside it. |
| `is not the file our render was made from` | the two sides used different `ab_input.wav`. Regenerate and re-render **both**. |
| mapping warning `...was clamped to...` | the prototype value is outside our parameter's range. The two sides are **not** running the same setting on that control; pick a value inside the range and re-render both. |
| our render peaks above full scale | export the Max side as float, or it will clip where ours does not. |

---

## Files

| file | what it is |
|---|---|
| `gen_input.py` | the deterministic test signal, and its SHA-256 |
| `ab_wav.py` | WAV read/write (float32, and the PCM depths Max may hand you) |
| `ab_render.cpp` | the renderer: input WAV + params JSON → our WAV + sidecar |
| `ab_mapping.h` | **the parameter mapping and the expected-difference table** — the single source of truth for both |
| `sha256.h` | copied from `offline-render` (which this must not edit) so the fingerprints agree |
| `CMakeLists.txt` | the console app, in the `offline-render` house style |
| `ab_compare.py` | alignment + the three metrics + the verdicts |
| `ab_selftest.py` | proves the above works, without needing a Max render |
| `params/*.json` | one worked example per prototype, with the defect remedies already applied |

Build output (`build/`), renders (`*.wav`, `out/`) and sidecars (`*.map.json`) are
git-ignored. `ab_input.wav` is reproducible from `gen_input.py`, so it is not committed
either — regenerate it rather than copying it between machines, and check the hash.

---

## The whole thing, as five lines

From `tools/validation/effects-ab`, once Max has given you `max_out.wav`:

```
python gen_input.py                                     # note the sha256 it prints
build\ab-render_artefacts\Release\ab-render.exe --print-mapping fx_delay
  ... render in Max, write down the inlet values in params/delay.json ...
build\ab-render_artefacts\Release\ab-render.exe --params params/delay.json ^
      --in ab_input.wav --out out/delay.ours.wav --expect-sha <hash>
python ab_compare.py --max max_out.wav --ours out/delay.ours.wav --input ab_input.wav
```

---

## Reproducibility, honestly stated

The modules use `FastDecibels` (libm-free) and `FrDiffusion::hashNoiseBipolar` for
randomness, so a render is reproducible across platforms **for those parts**. But
`std::tanh`, `std::cos`, `std::exp` and `std::tan` are used in the distortion, the
phaser, the LFOs and every filter coefficient, and those are not guaranteed identical
across libm implementations. A render made on Windows will not necessarily be
bit-identical to one made on macOS or Linux. That is why `offline-render` keeps one
baseline file per machine, and it is why this harness compares *renders you made
yourself* rather than checking a committed hash. At the −30/−40 dB targets here the
libm differences are some 140 dB below the noise floor of the question being asked.
