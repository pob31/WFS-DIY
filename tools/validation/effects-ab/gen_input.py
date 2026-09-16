#!/usr/bin/env python3
"""gen_input.py - the deterministic A/B input signal (effects plan section 10).

Writes one mono 32-bit-float WAV that the user drops into Max and that
`ab-render` reads on our side. Both renders must start from the SAME file, so
the tool prints a SHA-256 of the sample bytes; `ab-render` prints the same
fingerprint for the file it read, and `ab_compare.py` refuses a pair whose
fingerprints disagree.

Every sample is a pure function of its index. There is no RNG object anywhere:
the noise is the squirrel hash from spatcore/dsp/FrDiffusionModel.h
(scenarios.h:28-38 duplicates it the same way), evaluated in exact uint32
arithmetic, so the file is bit-identical on Windows, macOS and Linux and does
not change when Python, numpy or the platform does.

The content is chosen for what these nine modules actually respond to:

    0.00 - 0.20   silence           lead-in; also the comparator's zero reference
    0.20          impulse           +0.9, one sample - filters, delays, latency
    0.20 - 0.60   silence           the impulse response, alone
    0.60 - 1.00   step              DC +0.5 and back - DC blockers, envelopes
    1.00 - 1.20   silence
    1.20 - 3.60   sine bank         8 tones, 0.3 s each, 55 Hz -> 7040 Hz
    3.60 - 5.60   modulation probe  2.0 s of steady 1 kHz - the LFO window
    5.60 - 6.60   log sweep         20 Hz -> 20 kHz - shelves, cuts, phaser
    6.60 - 8.00   decaying bursts   4 x 1 kHz, tau 60 ms, 25 dB apart
    8.00 - 8.60   two tones         220 + 3150 Hz - intermodulation
    8.60 - 9.20   noise             hash noise, full band
    9.20 - 9.60   level step        -48 dBFS noise then -6 dBFS - gates
    9.60 - 10.00  silence tail      tails ring out; gives the aligner a tail

The steady 1 kHz probe exists so the modulation-rate and modulation-depth
metrics have a stationary carrier to measure an envelope on. Do not shorten it
below three periods of the slowest LFO you intend to test.

usage: python gen_input.py [--out ab_input.wav] [--sr 48000] [--seconds 10.0]
                           [--print-plan]
"""

import argparse
import hashlib
import os
import sys

import numpy as np

import ab_wav

# ---------------------------------------------------------------------------
# Squirrel hash - spatcore/dsp/FrDiffusionModel.h:52-63, uint32 throughout.
# ---------------------------------------------------------------------------

_M1 = np.uint32(0xB5297A4D)
_M2 = np.uint32(0x68E31DA4)
_M3 = np.uint32(0x1B56C4E9)


def hash_noise_bipolar(n, key):
    """Bipolar -1..+1 noise, bit-identical to FrDiffusion::hashNoiseBipolar."""
    n = np.asarray(n, dtype=np.uint32).copy()
    k = np.uint32(key)
    n *= _M1
    n += k
    n ^= n >> np.uint32(8)
    n += _M2
    n ^= n << np.uint32(8)
    n *= _M3
    n ^= n >> np.uint32(8)
    return n.view(np.int32).astype(np.float64) * (1.0 / 2147483648.0)


def make_key(a, b):
    """FrDiffusion::makeKey. Computed in Python ints and masked to 32 bits:
    numpy raises an overflow warning on a wrapping uint32 scalar multiply even
    though the wrapped answer is the correct one."""
    a = int(a) & 0xFFFFFFFF
    b = int(b) & 0xFFFFFFFF
    return np.uint32((a * 0x9E3779B9 + ((b + 1) & 0xFFFFFFFF) * 0x85EBCA6B + 1) & 0xFFFFFFFF)


# ---------------------------------------------------------------------------
# Segment plan. Times in seconds; everything below is derived from this table.
# ---------------------------------------------------------------------------

SEGMENTS = [
    (0.00, 0.20, 'silence',     'lead-in / zero reference'),
    (0.20, 0.60, 'impulse',     '+0.9 at t = 0.20 s, then silence'),
    (0.60, 1.00, 'step',        'DC +0.5, released at 1.00 s'),
    (1.00, 1.20, 'silence',     'settle'),
    (1.20, 3.60, 'sine-bank',   '55 110 220 440 880 1760 3520 7040 Hz, 0.3 s each'),
    (3.60, 5.60, 'mod-probe',   'steady 1 kHz - the LFO analysis window'),
    (5.60, 6.60, 'sweep',       'log sweep 20 Hz -> 20 kHz'),
    (6.60, 8.00, 'bursts',      '4 decaying 1 kHz bursts, tau 60 ms, 25 dB apart'),
    (8.00, 8.60, 'two-tone',    '220 + 3150 Hz'),
    (8.60, 9.20, 'noise',       'hash noise at -12 dBFS'),
    (9.20, 9.60, 'level-step',  'noise -48 dBFS then -6 dBFS'),
    (9.60, 10.00, 'silence',    'tail'),
]

#: Default analysis window for the modulation metrics, inset from the probe
#: segment by its 5 ms fades plus a settling margin.
MOD_WINDOW = (3.70, 5.55)

BANK_FREQS = [55.0, 110.0, 220.0, 440.0, 880.0, 1760.0, 3520.0, 7040.0]
PROBE_HZ = 1000.0
BURST_PEAKS = [0.9, 0.16, 0.9, 0.05]        # 15 dB, 25 dB, 25 dB apart
BURST_TAU = 0.060
EDGE_SECONDS = 0.005


def _edge(n, edge_samples):
    """Raised-cosine fade in/out over a run of n samples."""
    w = np.ones(n)
    e = min(edge_samples, n // 2)
    if e <= 0:
        return w
    ramp = 0.5 - 0.5 * np.cos(np.pi * (np.arange(e) + 0.5) / e)
    w[:e] = ramp
    w[n - e:] = ramp[::-1]
    return w


def build(sr=48000, seconds=10.0):
    """Return the input signal as float64, length round(sr * seconds)."""
    total = int(round(sr * seconds))
    x = np.zeros(total, dtype=np.float64)
    edge = int(round(EDGE_SECONDS * sr))

    def span(t0, t1):
        a = int(round(t0 * sr))
        b = int(round(t1 * sr))
        return max(0, min(a, total)), max(0, min(b, total))

    def tone(t0, t1, hz, amp):
        a, b = span(t0, t1)
        if b <= a:
            return
        n = np.arange(b - a, dtype=np.float64)
        x[a:b] += amp * np.sin(2.0 * np.pi * hz * n / sr) * _edge(b - a, edge)

    # impulse -------------------------------------------------------------
    a, _ = span(0.20, 0.60)
    if a < total:
        x[a] += 0.9

    # step ----------------------------------------------------------------
    a, b = span(0.60, 1.00)
    x[a:b] += 0.5

    # sine bank -----------------------------------------------------------
    for i, hz in enumerate(BANK_FREQS):
        t0 = 1.20 + 0.30 * i
        tone(t0, t0 + 0.30, hz, 0.5)

    # modulation probe ----------------------------------------------------
    tone(3.60, 5.60, PROBE_HZ, 0.5)

    # logarithmic sweep ---------------------------------------------------
    a, b = span(5.60, 6.60)
    if b > a:
        n = b - a
        t = np.arange(n, dtype=np.float64) / sr
        dur = n / sr
        f0, f1 = 20.0, 20000.0
        k = np.log(f1 / f0)
        phase = 2.0 * np.pi * f0 * dur / k * (np.exp(k * t / dur) - 1.0)
        x[a:b] += 0.5 * np.sin(phase) * _edge(n, edge)

    # decaying bursts -----------------------------------------------------
    for i, peak in enumerate(BURST_PEAKS):
        t0 = 6.60 + 0.35 * i
        a, b = span(t0, t0 + 0.35)
        if b <= a:
            continue
        n = np.arange(b - a, dtype=np.float64)
        env = peak * np.exp(-(n / sr) / BURST_TAU)
        x[a:b] += env * np.sin(2.0 * np.pi * 1000.0 * n / sr)

    # two tones -----------------------------------------------------------
    tone(8.00, 8.60, 220.0, 0.3)
    tone(8.00, 8.60, 3150.0, 0.3)

    # noise ---------------------------------------------------------------
    a, b = span(8.60, 9.20)
    if b > a:
        idx = np.arange(a, b, dtype=np.uint32)
        x[a:b] += 0.25 * hash_noise_bipolar(idx, make_key(7, 1)) * _edge(b - a, edge)

    # level step ----------------------------------------------------------
    a, b = span(9.20, 9.40)
    if b > a:
        idx = np.arange(a, b, dtype=np.uint32)
        x[a:b] += 0.004 * hash_noise_bipolar(idx, make_key(7, 2))
    a, b = span(9.40, 9.60)
    if b > a:
        idx = np.arange(a, b, dtype=np.uint32)
        x[a:b] += 0.5 * hash_noise_bipolar(idx, make_key(7, 3)) * _edge(b - a, edge)

    return x


def fingerprint(samples):
    """SHA-256 of the float32 little-endian sample bytes.

    The same bytes `ab-render` hashes, so the two sides can prove they started
    from the same file without either trusting the other's file name.
    """
    return hashlib.sha256(np.asarray(samples, dtype='<f4').tobytes()).hexdigest()


def main(argv=None):
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0],
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--out', default=os.path.join(here, 'ab_input.wav'))
    ap.add_argument('--sr', type=float, default=48000.0)
    ap.add_argument('--seconds', type=float, default=10.0)
    ap.add_argument('--print-plan', action='store_true',
                    help='list the segment table and exit')
    args = ap.parse_args(argv)

    if args.print_plan:
        print('%-8s %-8s %-12s %s' % ('start', 'end', 'segment', 'content'))
        for t0, t1, name, what in SEGMENTS:
            print('%-8.2f %-8.2f %-12s %s' % (t0, t1, name, what))
        print()
        print('modulation window: %.2f .. %.2f s' % MOD_WINDOW)
        return 0

    if args.sr <= 0 or args.seconds <= 0:
        print('error: --sr and --seconds must be positive', file=sys.stderr)
        return 2

    x = build(int(round(args.sr)), args.seconds)
    ab_wav.write_float32(args.out, x, int(round(args.sr)))

    peak = float(np.max(np.abs(x))) if len(x) else 0.0
    print('wrote   %s' % args.out)
    print('  rate  %d Hz, %d samples (%.3f s), mono, 32-bit float'
          % (int(round(args.sr)), len(x), len(x) / args.sr))
    print('  peak  %.6f (%.2f dBFS)'
          % (peak, 20.0 * np.log10(peak) if peak > 0 else -999.0))
    print('  sha256 %s' % fingerprint(x))
    return 0


if __name__ == '__main__':
    sys.exit(main())
