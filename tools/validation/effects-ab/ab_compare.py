#!/usr/bin/env python3
"""ab_compare.py - judge our render against the Max render (effects plan section 10).

    python ab_compare.py --max max_out.wav --ours out/delay.ours.wav

Aligns the two (they may differ by a latency, and by whatever offset the Max
render started at), then reports three things with a verdict each:

    RMS residual          the whole-file difference, relative to the Max render.
                          Target: better than -40 dB for a linear module,
                          better than -30 dB for a nonlinear one.
    1/3-octave spectral   the worst band-energy difference over 25 Hz..20 kHz,
                          reported both as a band error in dB and as the
                          equivalent residual so the SAME -40/-30 dB target
                          applies. A band ratio of r dB is a residual of
                          20*log10(|10^(r/20) - 1|): -40 dB is 0.087 dB of band
                          error, -30 dB is 0.274 dB.
    modulation            LFO rate and depth measured from the envelope of the
                          steady 1 kHz probe in the input file. Targets: rate
                          within 1 %, depth within 0.5 dB.

THE PART THAT MATTERS MOST: the sidecar that ab_render wrote next to our WAV
lists the prototype DEFECTS in force for this parameter set (plan section 5.13
P1..P4, plus the divergences ab_mapping.h records). On the parameters they
touch a LARGE difference is the correct answer, not a regression, so a metric
they affect is reported as EXPECTED-DIFF rather than FAIL and the reason is
printed. --strict turns that off, for when you have neutralised the defects in
Max and want the raw verdict.

exit codes: 0 everything passed or was expected, 1 a real failure,
            2 usage / unreadable input, 3 the two renders disagree about the
            input file they started from
"""

import argparse
import json
import math
import os
import sys

import numpy as np

import ab_wav

# Targets, plan section 10.
TARGET_LINEAR_DB = -40.0
TARGET_NONLINEAR_DB = -30.0
TARGET_MOD_RATE_PCT = 1.0
TARGET_MOD_DEPTH_DB = 0.5

#: Bands more than this far below the reference's loudest band carry no useful
#: information about a residual and are excluded from the spectral verdict.
SPECTRAL_FLOOR_DB = 60.0

#: Below this the envelope spectrum has no peak worth calling a modulation.
MOD_PEAK_PROMINENCE_DB = 6.0

# A comparison is only a comparison if it actually compared something.
# The adversarial review found two ways to reach 'all metrics passed' having
# measured nothing: a max_out.wav holding 1 s of a 10 s render (sfrecord~ was
# stopped early), and two silent files. Both now stop the run.
MIN_SPAN_FRACTION = 0.98   # of our render, after the alignment shift
MIN_VERDICTS = 2           # fewer than this and no verdict is being reported

#: And below this the "modulation" is envelope noise. A module with no LFO at
#: all still has a faintly rippled envelope, and reporting a rate for it would
#: put a meaningless metric beside three meaningful ones.
MOD_MIN_DEPTH_DB = 0.1

NEG_INF = float('-inf')


# ---------------------------------------------------------------------------
def db(x, floor=-999.0):
    return 20.0 * math.log10(x) if x > 0 else floor


def fmt_db(x):
    if x == NEG_INF or x <= -998.0:
        return '  -inf dB'
    return '%6.2f dB' % x


def ratio_db_to_residual_db(band_error_db):
    """A band magnitude ratio of r dB, expressed as a residual-to-reference dB.

    This is what lets one target serve both metrics: a band that is r dB hot
    contributes a difference of |10^(r/20) - 1| times the reference.
    """
    lin = abs(10.0 ** (band_error_db / 20.0) - 1.0)
    return NEG_INF if lin == 0.0 else db(lin)


# ---------------------------------------------------------------------------
def align(ours, ref, max_lag):
    """Integer lag (samples) that best aligns `ours` onto `ref`, by FFT cross
    correlation over the +-max_lag window, plus a parabolic sub-sample refinement
    reported for information.

    A positive lag means our render is LATE: ours[lag:] lines up with ref[0:].
    """
    n = max(len(ours), len(ref))
    size = 1
    while size < 2 * n:
        size *= 2

    fa = np.fft.rfft(ours, size)
    fb = np.fft.rfft(ref, size)
    cc = np.fft.irfft(fa * np.conj(fb), size)

    # cc[k] is the correlation at lag +k; cc[size-k] at lag -k.
    lags = np.arange(-max_lag, max_lag + 1)
    vals = np.empty(len(lags))
    for i, l in enumerate(lags):
        vals[i] = cc[l % size]

    best = int(np.argmax(vals))
    lag = int(lags[best])

    frac = 0.0
    if 0 < best < len(vals) - 1:
        y0, y1, y2 = vals[best - 1], vals[best], vals[best + 1]
        den = y0 - 2.0 * y1 + y2
        if den != 0.0:
            frac = 0.5 * (y0 - y2) / den
            frac = max(-1.0, min(1.0, frac))

    return lag, lag + frac


def overlap(ours, ref, lag):
    """Apply an integer lag and return the common span of both."""
    if lag > 0:
        a = ours[lag:]
        b = ref[:len(a)]
    elif lag < 0:
        b = ref[-lag:]
        a = ours[:len(b)]
    else:
        a, b = ours, ref
    n = min(len(a), len(b))
    return a[:n], b[:n]


# ---------------------------------------------------------------------------
def welch_psd(x, sr, nfft=8192):
    """Averaged periodogram. Plain numpy - scipy is not installed on the dev box."""
    if len(x) < nfft:
        nfft = 1 << max(8, int(math.floor(math.log2(max(len(x), 256)))))
    hop = nfft // 2
    win = np.hanning(nfft)
    scale = 1.0 / (sr * np.sum(win ** 2))

    acc = np.zeros(nfft // 2 + 1)
    count = 0
    for start in range(0, len(x) - nfft + 1, hop):
        seg = x[start:start + nfft] * win
        spec = np.fft.rfft(seg)
        acc += (np.abs(spec) ** 2) * scale
        count += 1

    if count == 0:
        return np.fft.rfftfreq(nfft, 1.0 / sr), acc
    return np.fft.rfftfreq(nfft, 1.0 / sr), acc / count


def third_octave_bands(sr):
    """ISO third-octave centres from 25 Hz up to the last one under Nyquist."""
    out = []
    n = -16                     # 1000 * 2^(-16/3) = 25 Hz
    while True:
        centre = 1000.0 * (2.0 ** (n / 3.0))
        lo = centre / (2.0 ** (1.0 / 6.0))
        hi = centre * (2.0 ** (1.0 / 6.0))
        if centre > 20000.0 or hi >= sr * 0.5:
            break
        out.append((centre, lo, hi))
        n += 1
    return out


def band_energies(freqs, psd, bands):
    out = np.zeros(len(bands))
    for i, (_c, lo, hi) in enumerate(bands):
        sel = (freqs >= lo) & (freqs < hi)
        out[i] = float(np.sum(psd[sel])) if np.any(sel) else 0.0
    return out


def spectral_error(a, b, sr):
    """Worst 1/3-octave band error, over the bands the reference actually uses."""
    bands = third_octave_bands(sr)
    fa, pa = welch_psd(a, sr)
    fb, pb = welch_psd(b, sr)
    ea = band_energies(fa, pa, bands)
    eb = band_energies(fb, pb, bands)

    if not np.any(eb > 0):
        return None

    floor = np.max(eb) * (10.0 ** (-SPECTRAL_FLOOR_DB / 10.0))
    used = eb > floor
    if not np.any(used):
        return None

    errs = np.zeros(len(bands))
    for i in range(len(bands)):
        if not used[i]:
            continue
        if eb[i] <= 0.0:
            errs[i] = 0.0
        elif ea[i] <= 0.0:
            errs[i] = -999.0
        else:
            errs[i] = 10.0 * math.log10(ea[i] / eb[i])

    idx = int(np.argmax(np.abs(np.where(used, errs, 0.0))))
    weights = eb[used] / np.sum(eb[used])
    rms_err = float(math.sqrt(np.sum(weights * (errs[used] ** 2))))

    return {
        'worstBandHz': bands[idx][0],
        'worstBandErrorDb': float(errs[idx]),
        'weightedRmsErrorDb': rms_err,
        'bandsUsed': int(np.sum(used)),
        'bandsTotal': len(bands),
    }


# ---------------------------------------------------------------------------
def envelope(x, block_hz, sr):
    """Block RMS - the envelope of the steady carrier, sampled at `block_hz`.

    RMS over a whole number of blocks is phase-independent and rejects the
    rectified carrier's ripple far better than a one-pole on |x|, which at a
    1 kHz carrier leaves several per cent of 2 kHz ripple sitting on top of the
    depth measurement. Non-overlapping blocks are enough: 500 Hz of envelope is
    twenty samples per cycle of the fastest LFO this metric looks for.
    """
    L = max(2, int(round(sr / block_hz)))
    n = (len(x) // L) * L
    if n < 4 * L:
        return None, 0.0
    e = np.sqrt(np.mean(x[:n].reshape(-1, L).astype(np.float64) ** 2, axis=1))
    return e, sr / L


def _refine_peak(sig, esr, f0, span, points=201):
    """Maximise the DTFT magnitude on a fine grid around f0, then parabolic.

    The FFT bin spacing over a two-second window is about 0.5 Hz, and the
    target is 1 % of an LFO that may be at 4 Hz - 0.04 Hz. A bin is therefore
    more than ten times too coarse, and parabolic interpolation on bins that
    coarse is not enough either. A Hann-windowed sinusoid's DTFT main lobe is
    symmetric about the true frequency, so evaluating it directly on a fine
    grid recovers the frequency to well under the target.
    """
    lo = max(0.0, f0 - span)
    hi = f0 + span
    grid = np.linspace(lo, hi, points)
    n = np.arange(len(sig), dtype=np.float64)
    phase = np.exp(-2.0j * np.pi * np.outer(grid, n) / esr)
    mags = np.abs(phase @ sig)

    k = int(np.argmax(mags))
    f = float(grid[k])
    if 0 < k < len(grid) - 1:
        y0, y1, y2 = mags[k - 1], mags[k], mags[k + 1]
        den = y0 - 2.0 * y1 + y2
        if den != 0.0:
            delta = 0.5 * (y0 - y2) / den
            if -1.0 < delta < 1.0:
                f += delta * (grid[1] - grid[0])
    return f


def measure_modulation(x, sr, t0, t1, block_hz=500.0):
    """LFO rate (Hz) and depth (dB) from the envelope of a stationary segment."""
    a = int(round(t0 * sr))
    b = int(round(t1 * sr))
    if b <= a or a >= len(x):
        return None
    seg = x[a:min(b, len(x))]
    if len(seg) < sr * 0.2:
        return None

    env, esr = envelope(seg, block_hz, sr)
    if env is None or len(env) < 64:
        return None

    mean = float(np.mean(env))
    if mean <= 0.0:
        return None

    ac = (env - mean) * np.hanning(len(env))

    # Coarse peak from a zero-padded FFT, then a fine DTFT search around it.
    pad = 1
    while pad < 16 * len(ac):
        pad *= 2
    spec = np.abs(np.fft.rfft(ac, pad))
    freqs = np.fft.rfftfreq(pad, 1.0 / esr)

    sel = (freqs >= 0.05) & (freqs <= 25.0)
    if not np.any(sel):
        return None

    idx_all = np.nonzero(sel)[0]
    local = spec[idx_all]
    k = int(idx_all[int(np.argmax(local))])

    peak = float(spec[k])
    median = float(np.median(local)) if len(local) else 0.0
    prominence = db(peak / median) if (median > 0 and peak > 0) else -999.0

    bin_hz = esr / len(ac)
    rate = _refine_peak(ac, esr, float(freqs[k]), 1.5 * bin_hz)

    # Depth over a whole number of LFO periods, so the two ends balance, and
    # from robust extremes so one glitch cannot set it.
    e = env
    if rate > 0:
        period = esr / rate
        periods = int(len(env) / period)
        if periods >= 1:
            keep = int(periods * period)
            if keep >= 64:
                e = env[:keep]

    hi = float(np.percentile(e, 99.0))
    lo = float(np.percentile(e, 1.0))
    depth = db(hi / lo) if lo > 0 else 999.0

    return {
        'rateHz': rate,
        'depthDb': depth,
        'prominenceDb': prominence,
        'detected': (prominence >= MOD_PEAK_PROMINENCE_DB
                     and depth >= MOD_MIN_DEPTH_DB),
    }



# ---------------------------------------------------------------------------
class Verdict:
    def __init__(self, name, value, target, ok, unit='dB', lower_is_better=True):
        self.name = name
        self.value = value
        self.target = target
        self.ok = ok
        self.unit = unit
        self.status = 'PASS' if ok else 'FAIL'
        self.excused_by = []

    def excuse(self, diffs):
        if self.status == 'FAIL' and diffs:
            self.status = 'EXPECTED-DIFF'
            self.excused_by = diffs


def diffs_for(metric, active, strict):
    """The `large` expected differences that cover this metric."""
    if strict:
        return []
    out = []
    for d in active:
        if d.get('severity') != 'large':
            continue
        metrics = [m.strip() for m in (d.get('metrics') or '').split(',') if m.strip()]
        if metric in metrics:
            out.append(d)
    return out


# ---------------------------------------------------------------------------
def main(argv=None):
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0],
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--max', dest='max_path', required=True,
                    help='max_out.wav - the render exported from Max')
    ap.add_argument('--ours', required=True, help='the WAV ab-render wrote')
    ap.add_argument('--sidecar', default=None,
                    help='the .map.json ab-render wrote (default: beside --ours)')
    ap.add_argument('--input', default=None,
                    help='ab_input.wav, to prove both sides started from one file')
    ap.add_argument('--max-lag-ms', type=float, default=250.0)
    ap.add_argument('--mod-window', nargs=2, type=float, default=[3.70, 5.55],
                    metavar=('T0', 'T1'),
                    help='the steady-tone window the LFO metrics measure (seconds)')
    ap.add_argument('--target-db', type=float, default=None,
                    help='override the residual target (default: from the sidecar)')
    ap.add_argument('--strict', action='store_true',
                    help='report every difference as FAIL, including the expected ones')
    ap.add_argument('--min-span', type=float, default=MIN_SPAN_FRACTION,
                    metavar='FRAC',
                    help='the fraction of our render the Max export must cover'
                         ' (default %.2f); below it the run is an error'
                         % MIN_SPAN_FRACTION)
    ap.add_argument('--allow-short-span', action='store_true',
                    help='downgrade a short Max export from an error to a warning'
                         ' - only when you deliberately exported a shorter excerpt')
    ap.add_argument('--allow-gain', action='store_true',
                    help='also report the residual after removing a best-fit level match')
    ap.add_argument('--json', dest='json_path', default=None,
                    help='write the full report as JSON as well')
    args = ap.parse_args(argv)

    # ---------------------------------------------------------------- load
    try:
        mx, mx_sr = ab_wav.read(args.max_path)
        ou, ou_sr = ab_wav.read(args.ours)
    except Exception as exc:
        print('error: %s' % exc, file=sys.stderr)
        return 2

    mx = ab_wav.mono(mx)
    ou = ab_wav.mono(ou)

    if abs(mx_sr - ou_sr) > 0.5:
        print('error: the two renders are at different sample rates (%d vs %d).\n'
              '       Set Max\'s DSP rate to match, or re-render ours with --sr.'
              % (mx_sr, ou_sr), file=sys.stderr)
        return 2

    sr = float(ou_sr)

    sidecar_path = args.sidecar
    if sidecar_path is None:
        base = os.path.splitext(args.ours)[0]
        cand = base + '.map.json'
        sidecar_path = cand if os.path.exists(cand) else None

    side = None
    if sidecar_path and os.path.exists(sidecar_path):
        with open(sidecar_path, 'r', encoding='utf-8') as f:
            side = json.load(f)

    active = (side or {}).get('expectedDifferences', [])
    nonlinear = bool((side or {}).get('nonlinear', False))
    target = args.target_db if args.target_db is not None else (
        TARGET_NONLINEAR_DB if nonlinear else TARGET_LINEAR_DB)

    # --------------------------------------------------- input fingerprint
    if args.input and side:
        try:
            import gen_input
            xin, _ = ab_wav.read(args.input)
            sha = gen_input.fingerprint(ab_wav.mono(xin))
        except Exception as exc:
            print('error: cannot fingerprint %s (%s)' % (args.input, exc), file=sys.stderr)
            return 2
        want = side.get('input', {}).get('sha256')
        if want and want != sha:
            print('error: %s is not the file our render was made from.\n'
                  '       ours started from sha256 %s\n'
                  '       this file is           %s\n'
                  '       Regenerate with gen_input.py and re-render BOTH sides.'
                  % (args.input, want, sha), file=sys.stderr)
            return 3

    # ------------------------------------------------------------- header
    print('=' * 78)
    print('A/B  %s' % ((side or {}).get('patch', '(no sidecar - patch unknown)')))
    print('=' * 78)
    print('  max   %s  %d samples' % (args.max_path, len(mx)))
    print('  ours  %s  %d samples' % (args.ours, len(ou)))
    print('  rate  %.0f Hz' % sr)
    if side:
        print('  class %s -> residual target %.0f dB'
              % ('nonlinear' if nonlinear else 'linear', target))
        print('  our module latency: %d samples' % side.get('latencySamples', 0))
    else:
        print('  no sidecar found: assuming the %s target and no expected differences.'
              % ('nonlinear' if nonlinear else 'linear'))
        print('  Pass --sidecar, or the defect list cannot be honoured.')
    if args.strict:
        print('  --strict: expected differences will be reported as FAIL')
    print()

    # ---------------------------------------------------------- alignment
    max_lag = max(1, int(round(args.max_lag_ms * 0.001 * sr)))
    lag, frac_lag = align(ou, mx, max_lag)
    a, b = overlap(ou, mx, lag)

    if len(a) < 16:
        print('error: the two renders do not overlap after alignment', file=sys.stderr)
        return 2

    print('  alignment')
    print('    lag            %+d samples (%+.3f ms), sub-sample estimate %+.2f'
          % (lag, lag * 1000.0 / sr, frac_lag))
    if abs(lag) >= max_lag:
        print('    ! the lag hit the +-%d sample search limit - raise --max-lag-ms'
              % max_lag)
    # --------------------------------------------- did we compare the whole run?
    # overlap() trims to the shorter of the two, so a Max export that stops early
    # silently shrinks the comparison. Report the coverage every time, and refuse
    # to hand out a verdict when most of the render was never looked at.
    required = max(1, len(ou) - abs(lag))
    coverage = len(a) / float(required)
    print('    compared span  %d samples (%.3f s) = %.1f%% of our render'
          % (len(a), len(a) / sr, coverage * 100.0))

    if coverage < args.min_span:
        lines = ['the Max export covers only %.1f%% of our render (%.3f s of'
                 ' %.3f s).' % (coverage * 100.0, len(a) / sr, required / sr),
                 'The usual cause is sfrecord~ being stopped before the input'
                 ' finished, so most', 'of the render was never compared.'
                 ' Re-export the full %.2f s.' % (len(ou) / sr)]
        if not args.allow_short_span:
            print('error: ' + lines[0], file=sys.stderr)
            for line in lines[1:]:
                print('       ' + line, file=sys.stderr)
            print('       Pass --allow-short-span if the excerpt really is what you'
                  ' want, or --min-span FRAC', file=sys.stderr)
            print('       to move the bar.', file=sys.stderr)
            return 4
        print('    ! WARNING: ' + lines[0])
        for line in lines[1:]:
            print('      ' + line)
        short_span_note = ('only %.1f%% of our render was compared'
                           % (coverage * 100.0))
    else:
        short_span_note = None

    ref_rms = float(np.sqrt(np.mean(b ** 2)))
    resid = a - b
    res_rms = float(np.sqrt(np.mean(resid ** 2)))

    ours_rms = float(np.sqrt(np.mean(a ** 2)))

    # ------------------------------------------- is there anything to compare?
    # Every metric below is a RATIO against the reference. With a silent
    # reference the residual is -inf, the spectrum has no usable band and the
    # LFO probe returns n/a - and the old code printed 'all metrics passed'.
    # A silent max_out.wav is a broken export, never a passing port.
    if ref_rms <= 0.0:
        print('error: %s is silent over the compared span, so nothing can be'
              ' measured against it.' % args.max_path, file=sys.stderr)
        print('       Every metric here is a ratio to the Max render. Check that'
              ' sfrecord~ was', file=sys.stderr)
        print('       actually recording, that the gen~ patch was loaded, and that'
              ' the export is not', file=sys.stderr)
        print('       silence-padded at the point the alignment landed on.',
              file=sys.stderr)
        return 4

    if ours_rms <= 0.0:
        print('error: %s is silent over the compared span. Our own render produced'
              ' nothing -' % args.ours, file=sys.stderr)
        print('       re-run ab-render and check its reported peak/rms before'
              ' comparing.', file=sys.stderr)
        return 4

    gain = float(np.dot(a, b) / np.dot(b, b)) if np.dot(b, b) > 0 else 1.0
    if gain > 0.0:
        print('    level match    %+.3f dB (ours vs Max, least squares)' % db(gain))
    elif gain < 0.0:
        print('    level match    %+.3f dB AND INVERTED - check the polarity of the'
              ' Max export' % db(-gain))
    print()

    results = {}
    verdicts = []

    # -------------------------------------------------------- RMS residual
    # ref_rms > 0 is guaranteed above, so this is a real ratio in every branch.
    rms_db = NEG_INF if res_rms == 0.0 else db(res_rms / ref_rms)

    v = Verdict('RMS residual', rms_db, target, rms_db <= target)
    v.excuse(diffs_for('rms', active, args.strict))
    verdicts.append(v)
    results['rmsResidualDb'] = rms_db if rms_db != NEG_INF else None

    gain_line = None
    if args.allow_gain and gain != 0.0:
        r2 = a - gain * b
        rr = float(np.sqrt(np.mean(r2 ** 2)))
        gd = NEG_INF if rr == 0.0 else (db(rr / ref_rms) if ref_rms > 0 else 999.0)
        gain_line = gd
        results['rmsResidualAfterGainMatchDb'] = gd if gd != NEG_INF else None

    # ------------------------------------------------ 1/3-octave spectrum
    spectral_note = None
    spec = spectral_error(a, b, sr)
    if spec is not None:
        equiv = ratio_db_to_residual_db(spec['worstBandErrorDb'])
        # json has no -inf: an exact null is written as null, as the RMS one is.
        results['spectral'] = dict(spec,
                                   equivalentResidualDb=None if equiv == NEG_INF else equiv)
        v = Verdict('1/3-octave spectral', equiv, target, equiv <= target)
        v.extra = ('worst %+.3f dB at %.0f Hz, weighted rms %.3f dB, %d/%d bands'
                   % (spec['worstBandErrorDb'], spec['worstBandHz'],
                      spec['weightedRmsErrorDb'], spec['bandsUsed'], spec['bandsTotal']))
        v.excuse(diffs_for('spectral', active, args.strict))
        verdicts.append(v)
    else:
        results['spectral'] = None
        spectral_note = ('the Max render has no usable 1/3-octave band in the'
                         ' compared span, so this metric did NOT run')

    # ------------------------------------------------------- modulation
    t0, t1 = args.mod_window
    mod_a = measure_modulation(a, sr, t0, t1)
    mod_b = measure_modulation(b, sr, t0, t1)
    results['modulation'] = {'ours': mod_a, 'max': mod_b, 'window': [t0, t1]}

    mod_ready = (mod_a is not None and mod_b is not None
                 and mod_a['detected'] and mod_b['detected'])

    if mod_ready:
        rate_err = abs(mod_a['rateHz'] - mod_b['rateHz']) / mod_b['rateHz'] * 100.0
        depth_err = abs(mod_a['depthDb'] - mod_b['depthDb'])
        results['modulation']['rateErrorPct'] = rate_err
        results['modulation']['depthErrorDb'] = depth_err

        v = Verdict('LFO rate', rate_err, TARGET_MOD_RATE_PCT,
                    rate_err <= TARGET_MOD_RATE_PCT, unit='%')
        v.extra = 'ours %.4f Hz, max %.4f Hz' % (mod_a['rateHz'], mod_b['rateHz'])
        v.excuse(diffs_for('modRate', active, args.strict))
        verdicts.append(v)

        v = Verdict('LFO depth', depth_err, TARGET_MOD_DEPTH_DB,
                    depth_err <= TARGET_MOD_DEPTH_DB)
        v.extra = 'ours %.3f dB, max %.3f dB' % (mod_a['depthDb'], mod_b['depthDb'])
        v.excuse(diffs_for('modDepth', active, args.strict))
        verdicts.append(v)

    # ---------------------------------------------------------- report
    print('  metrics')
    for v in verdicts:
        value = '   -inf' if v.value == NEG_INF else '%7.3f' % v.value
        print('    %-20s %s %-2s  target %6.2f %-2s  %s'
              % (v.name, value, v.unit, v.target, v.unit, v.status))
        if getattr(v, 'extra', None):
            print('    %-20s   %s' % ('', v.extra))
        for d in v.excused_by:
            print('    %-20s   excused by [%s] %s' % ('', d['id'], d['title']))

    if gain_line is not None:
        print('    %-20s %7.3f dB  (informational: after a best-fit level match)'
              % ('  after gain match', gain_line if gain_line != NEG_INF else -999.0))

    if spectral_note is not None:
        print('    %-20s n/a          (%s)' % ('1/3-octave spectral',
                                              spectral_note))

    if not mod_ready:
        why = ('no modulation detected in one or both renders (needs a peak %.0f dB '
               'above the envelope floor and at least %.1f dB of depth)'
               % (MOD_PEAK_PROMINENCE_DB, MOD_MIN_DEPTH_DB))
        if mod_a is None or mod_b is None:
            why = ('the %.2f..%.2f s window gave no envelope to measure in one of the'
                   ' renders (it may be outside the file, too short, or silent)'
                   % (t0, t1))
        print('    %-20s n/a          (%s)' % ('LFO rate / depth', why))

    # -------------------------------------------- the defects, spelt out
    large = [d for d in active if d.get('severity') == 'large']
    small = [d for d in active if d.get('severity') != 'large']

    if large:
        print()
        print('  EXPECTED DIFFERENCES (plan section 5.13 and ab_mapping.h)')
        print('  These are why a metric above may read EXPECTED-DIFF. A large residual')
        print('  on one of these is the CORRECT result - do not change the module.')
        for d in large:
            print()
            print('    [%s] %s' % (d['id'], d['title']))
            print('      affects: %s' % (d.get('metrics') or '(informational)'))
            for line in wrap(d.get('why', ''), 72):
                print('      %s' % line)
            if d.get('remedy'):
                print('      in Max:')
                for line in wrap(d['remedy'], 70):
                    print('        %s' % line)

    if small:
        print()
        print('  minor known divergences (they do NOT excuse a failure):')
        for d in small:
            print('    [%s] %s' % (d['id'], d['title']))

    if side and side.get('warnings'):
        print()
        print('  MAPPING WARNINGS from the render - the two sides may not be running')
        print('  the same setting on these controls:')
        for w in side['warnings']:
            for line in wrap(w, 74):
                print('    %s' % line)

    # ------------------------------------------------------------ verdict
    failed = [v for v in verdicts if v.status == 'FAIL']
    excused = [v for v in verdicts if v.status == 'EXPECTED-DIFF']
    results['verdictCount'] = len(verdicts)
    results['spanCoverage'] = coverage

    # A run in which almost nothing produced a verdict must not read as success.
    # Metrics drop out silently - spectral_error() returns None, the LFO probe
    # returns n/a - and with enough of them gone 'all metrics passed' was being
    # printed over an empty list.
    if len(verdicts) < MIN_VERDICTS:
        print()
        print('  RESULT: INCONCLUSIVE - only %d of the metrics produced a verdict'
              ' (%d needed).' % (len(verdicts), MIN_VERDICTS))
        print('  Nothing above is evidence that the port is right. See the n/a lines'
              ' for why')
        print('  each metric dropped out.')
        print()
        if args.json_path:
            results['verdicts'] = [{'name': v.name,
                                    'value': None if v.value == NEG_INF else v.value,
                                    'target': v.target, 'unit': v.unit,
                                    'status': v.status,
                                    'excusedBy': [d['id'] for d in v.excused_by]}
                                   for v in verdicts]
            results['result'] = 'inconclusive'
            with open(args.json_path, 'w', encoding='utf-8') as f:
                json.dump(results, f, indent=2)
            print('  wrote %s' % args.json_path)
        return 4

    print()
    if failed:
        print('  RESULT: %d metric%s FAILED - %s'
              % (len(failed), '' if len(failed) == 1 else 's',
                 ', '.join(v.name for v in failed)))
    elif excused:
        print('  RESULT: everything either passed or is an EXPECTED difference'
              ' (%s)' % ', '.join(v.name for v in excused))
    else:
        print('  RESULT: all metrics passed')
    if short_span_note is not None:
        print('  CAVEAT: %s - the rest was never looked at.' % short_span_note)
    print()

    results['verdicts'] = [{'name': v.name,
                            'value': None if v.value == NEG_INF else v.value,
                            'target': v.target,
                            'unit': v.unit,
                            'status': v.status,
                            'excusedBy': [d['id'] for d in v.excused_by]}
                           for v in verdicts]
    results['lagSamples'] = lag
    results['levelMatchDb'] = db(abs(gain)) if gain != 0 else None
    results['sampleRate'] = sr
    results['strict'] = args.strict

    if args.json_path:
        with open(args.json_path, 'w', encoding='utf-8') as f:
            json.dump(results, f, indent=2)
        print('  wrote %s' % args.json_path)

    return 1 if failed else 0


def wrap(text, width):
    words = (text or '').split()
    lines, cur = [], ''
    for w in words:
        if not cur:
            cur = w
        elif len(cur) + 1 + len(w) <= width:
            cur += ' ' + w
        else:
            lines.append(cur)
            cur = w
    if cur:
        lines.append(cur)
    return lines or ['']


if __name__ == '__main__':
    sys.exit(main())
