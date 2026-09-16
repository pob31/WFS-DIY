#!/usr/bin/env python3
"""ab_selftest.py - exercise the whole harness without a Max render.

The user has not produced a Max export yet, so the real comparison cannot be
run. Everything else can, and a harness that has never run is worth nothing.
This drives the parts that exist and checks the numbers against values that are
known in advance:

  1. gen_input.py is deterministic: generating twice gives the same SHA-256.
  2. ab-render renders each of the seven prototype patches, and rendering the
     same one twice is bit-identical.
  3. ab_compare against OUR OWN RENDER: the residual must be exactly zero
     (-inf dB), the lag exactly 0, the spectral error exactly 0 dB.
  4. ab_compare against our render SCALED by a known amount: the residual must
     come out at the level that scaling implies, 20*log10(|g - 1|), to within
     0.01 dB.
  5. ab_compare against our render DELAYED by a known number of samples: the
     aligner must find exactly that lag and the residual must return to zero.
  6. ab_compare against our render plus NOISE at a known level: the residual
     must come out at that level, to within 0.2 dB.
  7. ab_compare against a render with a known LFO RATE difference: the rate
     metric must measure it and fail by the right amount.
  8. The expected-difference machinery: a metric a section 5.13 defect covers
     must report EXPECTED-DIFF rather than FAIL, and --strict must turn it back
     into FAIL.
  9. The mapping refuses what it must: an unknown inlet name, and fx_chorus's
     unmapped second delay line.

usage: python ab_selftest.py [--exe <ab-render.exe>] [--keep]
"""

import argparse
import hashlib
import json
import math
import os
import shutil
import subprocess
import sys
import tempfile

import numpy as np

import ab_wav
import gen_input

HERE = os.path.dirname(os.path.abspath(__file__))

DEFAULT_EXE = os.path.join(HERE, 'build', 'ab-render_artefacts', 'Release',
                           'ab-render.exe' if os.name == 'nt' else 'ab-render')

PATCHES = ['tremolo', 'bitcrusher', 'distortion', 'dynamics', 'chorus', 'flanger', 'delay']

_fails = []
_checks = 0


def check(name, ok, detail=''):
    global _checks
    _checks += 1
    print('  [%s] %s%s' % ('ok  ' if ok else 'FAIL', name, ('  -- ' + detail) if detail else ''))
    if not ok:
        _fails.append(name)
    return ok


def close(a, b, tol):
    return abs(a - b) <= tol


def run(cmd, expect=0):
    p = subprocess.run(cmd, capture_output=True, text=True)
    if expect is not None and p.returncode != expect:
        print('    command: %s' % ' '.join(cmd))
        print('    exit %d\n%s\n%s' % (p.returncode, p.stdout[-3000:], p.stderr[-3000:]))
    return p


def compare(work, ours, other, sidecar, extra=None):
    """Run ab_compare and return (exit code, parsed json report)."""
    out = os.path.join(work, 'report.json')
    cmd = [sys.executable, os.path.join(HERE, 'ab_compare.py'),
           '--max', other, '--ours', ours, '--sidecar', sidecar,
           '--json', out]
    if extra:
        cmd += extra
    p = subprocess.run(cmd, capture_output=True, text=True)
    report = None
    if os.path.exists(out):
        with open(out, 'r', encoding='utf-8') as f:
            report = json.load(f)
    return p, report


def metric(report, name):
    for v in report['verdicts']:
        if v['name'] == name:
            return v
    return None


# ---------------------------------------------------------------------------
def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0],
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--exe', default=DEFAULT_EXE)
    ap.add_argument('--keep', action='store_true', help='keep the scratch directory')
    args = ap.parse_args(argv)

    if not os.path.exists(args.exe):
        print('error: %s does not exist. Build it first:\n'
              '  cmake -S tools/validation/effects-ab -B tools/validation/effects-ab/build '
              '-G "Visual Studio 18 2026"\n'
              '  cmake --build tools/validation/effects-ab/build --config Release'
              % args.exe, file=sys.stderr)
        return 2

    work = tempfile.mkdtemp(prefix='effects-ab-selftest-')
    print('scratch: %s\n' % work)

    try:
        # ---------------------------------------------------- 1 determinism
        print('1. the input signal is deterministic')
        wav1 = os.path.join(work, 'in1.wav')
        wav2 = os.path.join(work, 'in2.wav')
        for w in (wav1, wav2):
            run([sys.executable, os.path.join(HERE, 'gen_input.py'), '--out', w])
        h1 = hashlib.sha256(open(wav1, 'rb').read()).hexdigest()
        h2 = hashlib.sha256(open(wav2, 'rb').read()).hexdigest()
        check('two runs of gen_input.py give the same file', h1 == h2, h1[:16])

        x, sr = ab_wav.read(wav1)
        x = ab_wav.mono(x)
        sha = gen_input.fingerprint(x)
        check('the file is 10 s of mono at 48 kHz',
              sr == 48000 and len(x) == 480000, '%d samples at %d Hz' % (len(x), sr))
        check('no sample exceeds full scale', float(np.max(np.abs(x))) <= 1.0,
              'peak %.6f' % float(np.max(np.abs(x))))
        check('the silent lead-in really is silent',
              float(np.max(np.abs(x[:int(0.2 * sr)]))) == 0.0)

        # ------------------------------------------------------ 2 rendering
        print('\n2. ab-render renders every prototype, reproducibly')
        renders = {}
        for name in PATCHES:
            params = os.path.join(HERE, 'params', '%s.json' % name)
            out_a = os.path.join(work, '%s.a.wav' % name)
            out_b = os.path.join(work, '%s.b.wav' % name)
            p = run([args.exe, '--params', params, '--in', wav1, '--out', out_a,
                     '--expect-sha', sha])
            if not check('%-11s renders' % name, p.returncode == 0):
                continue
            run([args.exe, '--params', params, '--in', wav1, '--out', out_b])
            same = open(out_a, 'rb').read() == open(out_b, 'rb').read()
            check('%-11s renders identically twice' % name, same)

            y = ab_wav.mono(ab_wav.read(out_a)[0])
            check('%-11s output is finite and not silent' % name,
                  bool(np.all(np.isfinite(y))) and float(np.max(np.abs(y))) > 0.0,
                  'peak %.4f' % float(np.max(np.abs(y))))
            renders[name] = (out_a, os.path.splitext(out_a)[0] + '.map.json')

        if 'delay' not in renders:
            print('\nnothing to compare - aborting')
            return 1

        ours, sidecar = renders['delay']
        y = ab_wav.mono(ab_wav.read(ours)[0])

        # ------------------------------------------------ 3 identity null
        print('\n3. comparing a render against ITSELF must null exactly')
        p, rep = compare(work, ours, ours, sidecar)
        check('exit code 0', p.returncode == 0, p.stderr.strip()[:200])
        if rep:
            check('lag is exactly 0', rep['lagSamples'] == 0, str(rep['lagSamples']))
            check('RMS residual is exactly zero', rep['rmsResidualDb'] is None,
                  'reported %s' % rep['rmsResidualDb'])
            sp = rep.get('spectral')
            check('worst 1/3-octave band error is 0.000 dB',
                  sp is not None and close(sp['worstBandErrorDb'], 0.0, 1e-9),
                  '%.3e dB' % sp['worstBandErrorDb'] if sp else 'no spectrum')
            check('every metric passes',
                  all(v['status'] == 'PASS' for v in rep['verdicts']),
                  ', '.join('%s=%s' % (v['name'], v['status']) for v in rep['verdicts']))

        # -------------------------------------------- 4 known level offset
        print('\n4. a known LEVEL change must read back as the level it is')
        for g_db in (-0.5, -6.0, -40.0):
            g = 10.0 ** (g_db / 20.0)
            altered = os.path.join(work, 'delay.gain%+g.wav' % g_db)
            ab_wav.write_float32(altered, (y * g).astype(np.float32), sr)

            # ours = y, reference = y*g, so the residual is |1 - g| * |y| and
            # the reference rms is g * rms(y): expected = 20log10(|1-g|/g).
            expected = 20.0 * math.log10(abs(1.0 - g) / g)
            p, rep = compare(work, ours, altered, sidecar)
            got = rep['rmsResidualDb'] if rep else None
            check('%+.1f dB offset -> residual %+.3f dB' % (g_db, expected),
                  got is not None and close(got, expected, 0.01),
                  'measured %.4f dB' % got if got is not None else 'none')
            if rep:
                check('  and the level match reports %+.3f dB' % (-g_db),
                      close(rep['levelMatchDb'], -g_db, 0.01),
                      '%.4f dB' % rep['levelMatchDb'])

        # ------------------------------------------------- 5 known latency
        print('\n5. a known LATENCY must be found and removed')
        for shift in (1, 37, 1000):
            delayed = np.concatenate([np.zeros(shift, dtype=np.float32), y[:-shift]])
            path = os.path.join(work, 'delay.shift%d.wav' % shift)
            ab_wav.write_float32(path, delayed, sr)

            # ours leads the reference by `shift`, so the aligner must report -shift.
            p, rep = compare(work, ours, path, sidecar)
            check('%4d-sample shift is found' % shift,
                  rep is not None and rep['lagSamples'] == -shift,
                  'reported %s' % (rep['lagSamples'] if rep else 'none'))
            check('  and the residual nulls after alignment',
                  rep is not None and rep['rmsResidualDb'] is None,
                  'residual %s dB' % (rep['rmsResidualDb'] if rep else '?'))

        # --------------------------------------------------- 6 known noise
        print('\n6. a known amount of added NOISE must read back at its level')
        ref_rms = float(np.sqrt(np.mean(y.astype(np.float64) ** 2)))
        for n_db in (-35.0, -60.0):
            amp = ref_rms * (10.0 ** (n_db / 20.0))
            idx = np.arange(len(y), dtype=np.uint32)
            noise = gen_input.hash_noise_bipolar(idx, gen_input.make_key(99, 1))
            noise = noise / float(np.sqrt(np.mean(noise ** 2))) * amp
            path = os.path.join(work, 'delay.noise%g.wav' % n_db)
            ab_wav.write_float32(path, (y + noise).astype(np.float32), sr)

            p, rep = compare(work, ours, path, sidecar)
            got = rep['rmsResidualDb'] if rep else None
            check('%+.0f dB of noise -> residual near %+.0f dB' % (n_db, n_db),
                  got is not None and close(got, n_db, 0.2),
                  'measured %.3f dB' % got if got is not None else 'none')

        # ------------------------------------------- 7 known LFO rate error
        print('\n7. a known LFO RATE error must be measured as that error')
        trem_params = os.path.join(work, 'trem4.json')
        with open(os.path.join(HERE, 'params', 'tremolo.json'), 'r', encoding='utf-8') as f:
            base = json.load(f)
        ref_wav = os.path.join(work, 'trem.ref.wav')
        base['prototype']['freq'] = 4.0
        with open(trem_params, 'w', encoding='utf-8') as f:
            json.dump(base, f)
        run([args.exe, '--params', trem_params, '--in', wav1, '--out', ref_wav])

        off_wav = os.path.join(work, 'trem.off.wav')
        base['prototype']['freq'] = 4.10             # +2.5 %
        with open(trem_params, 'w', encoding='utf-8') as f:
            json.dump(base, f)
        run([args.exe, '--params', trem_params, '--in', wav1, '--out', off_wav])

        off_side = os.path.splitext(off_wav)[0] + '.map.json'
        p, rep = compare(work, off_wav, ref_wav, off_side, extra=['--strict'])
        rate = metric(rep, 'LFO rate') if rep else None
        check('the LFO rate metric fires at all', rate is not None,
              'metrics: %s' % ([v['name'] for v in rep['verdicts']] if rep else 'none'))
        if rate:
            check('4.10 Hz against 4.00 Hz measures 2.5 per cent +-0.05',
                  close(rate['value'], 2.5, 0.05), '%.3f per cent' % rate['value'])
            check('and that is a FAIL against the 1 per cent target',
                  rate['status'] == 'FAIL', rate['status'])

        p, rep = compare(work, ref_wav, ref_wav,
                         os.path.splitext(ref_wav)[0] + '.map.json', extra=['--strict'])
        rate = metric(rep, 'LFO rate') if rep else None
        check('the same tremolo against itself passes the rate metric',
              rate is not None and rate['status'] == 'PASS' and rate['value'] == 0.0,
              '%.4f per cent %s' % (rate['value'], rate['status']) if rate else 'no metric')

        # A known DEPTH difference must show up on the depth metric and nowhere
        # else. 12 dB against 15 dB of tremolo depth, same rate.
        deep_wav = os.path.join(work, 'trem.deep.wav')
        base['prototype']['freq'] = 4.0
        base['prototype']['depth'] = 15.0
        with open(trem_params, 'w', encoding='utf-8') as f:
            json.dump(base, f)
        run([args.exe, '--params', trem_params, '--in', wav1, '--out', deep_wav])

        p, rep = compare(work, deep_wav, ref_wav,
                         os.path.splitext(deep_wav)[0] + '.map.json', extra=['--strict'])
        depth = metric(rep, 'LFO depth') if rep else None
        rate = metric(rep, 'LFO rate') if rep else None
        check('15 dB against 12 dB of depth reads about 3 dB apart',
              depth is not None and close(depth['value'], 3.0, 0.4),
              '%.3f dB' % depth['value'] if depth else 'no metric')
        check('and the RATE metric still passes - the two do not cross-talk',
              rate is not None and rate['status'] == 'PASS',
              '%.4f per cent' % rate['value'] if rate else 'no metric')

        # --------------------------------- 8 the expected-difference logic
        print('\n8. a section 5.13 defect excuses its metrics, and --strict does not')
        # fx_delay with the prototype's own 0 dB shelf gains and shelfGainNominal
        # on: P1, P2 and D-DELAY-SHELVES all come into force on rms + spectral.
        nominal = os.path.join(work, 'delay.nominal.json')
        with open(nominal, 'w', encoding='utf-8') as f:
            json.dump({'patch': 'fx_delay',
                       'options': {'shelfGainNominal': True},
                       'prototype': {'delayTime': 0.375, 'feedback': 40.0,
                                     'dryWet': 65.0, 'loShelfGain': 0.0,
                                     'hiShelfGain': 0.0, 'loShelfFreq': 200.0,
                                     'hiShelfFreq': 4000.0}}, f)
        nom_wav = os.path.join(work, 'delay.nominal.wav')
        p = run([args.exe, '--params', nominal, '--in', wav1, '--out', nom_wav])
        nom_side = os.path.splitext(nom_wav)[0] + '.map.json'

        with open(nom_side, 'r', encoding='utf-8') as f:
            side = json.load(f)
        ids = [d['id'] for d in side['expectedDifferences']]
        check('P1 and P2 are declared for a nominal-shelf delay render',
              'P1' in ids and 'P2' in ids, ', '.join(ids))

        # Compare it against a deliberately wrong copy so the metrics DO fail.
        bad = ab_wav.mono(ab_wav.read(nom_wav)[0]) * (10.0 ** (-3.0 / 20.0))
        bad_path = os.path.join(work, 'delay.nominal.bad.wav')
        ab_wav.write_float32(bad_path, bad.astype(np.float32), sr)

        p, rep = compare(work, nom_wav, bad_path, nom_side)
        rms = metric(rep, 'RMS residual') if rep else None
        check('with the defects declared, a failing residual reads EXPECTED-DIFF',
              rms is not None and rms['status'] == 'EXPECTED-DIFF',
              rms['status'] if rms else 'no metric')
        check('and it names the defect that excused it',
              rms is not None and 'P2' in rms['excusedBy'],
              ', '.join(rms['excusedBy']) if rms else '')

        p, rep = compare(work, nom_wav, bad_path, nom_side, extra=['--strict'])
        rms = metric(rep, 'RMS residual') if rep else None
        check('--strict turns the same result back into FAIL',
              rms is not None and rms['status'] == 'FAIL',
              rms['status'] if rms else 'no metric')
        check('--strict exits non-zero', p.returncode == 1, 'exit %d' % p.returncode)

        # ----------------------------------------- 9 the mapping refusals
        print('\n9. the mapping refuses what it cannot compare')
        typo = os.path.join(work, 'typo.json')
        with open(typo, 'w', encoding='utf-8') as f:
            json.dump({'patch': 'fx_delay',
                       'prototype': {'delaytime': 0.375}}, f)     # wrong case
        p = run([args.exe, '--params', typo, '--in', wav1,
                 '--out', os.path.join(work, 'nope.wav')], expect=5)
        check('a misspelt inlet name is refused, not defaulted',
              p.returncode == 5 and 'not an inlet' in p.stderr, 'exit %d' % p.returncode)

        line2 = os.path.join(work, 'line2.json')
        with open(line2, 'w', encoding='utf-8') as f:
            json.dump({'patch': 'fx_chorus',
                       'prototype': {'delaytime1': 720.0, 'delaytime2onOff': 1.0}}, f)
        p = run([args.exe, '--params', line2, '--in', wav1,
                 '--out', os.path.join(work, 'nope.wav')], expect=5)
        check("fx_chorus's unmapped second line is refused",
              p.returncode == 5 and 'SECOND delay line' in p.stderr, 'exit %d' % p.returncode)

        with open(line2, 'w', encoding='utf-8') as f:
            json.dump({'patch': 'fx_chorus',
                       'options': {'allowUnmapped': True},
                       'prototype': {'delaytime1': 720.0, 'delaytime2onOff': 1.0}}, f)
        p = run([args.exe, '--params', line2, '--in', wav1,
                 '--out', os.path.join(work, 'line2.wav')])
        check('and allowUnmapped lets it through anyway', p.returncode == 0,
              'exit %d' % p.returncode)

        wrong_input = os.path.join(work, 'other.wav')
        ab_wav.write_float32(wrong_input, np.zeros(1000, dtype=np.float32), sr)
        p = run([args.exe, '--params', os.path.join(HERE, 'params', 'delay.json'),
                 '--in', wrong_input, '--out', os.path.join(work, 'nope.wav'),
                 '--expect-sha', sha], expect=6)
        check('--expect-sha catches the wrong input file', p.returncode == 6,
              'exit %d' % p.returncode)

        p = subprocess.run([sys.executable, os.path.join(HERE, 'ab_compare.py'),
                            '--max', ours, '--ours', ours, '--sidecar', sidecar,
                            '--input', wrong_input], capture_output=True, text=True)
        check('ab_compare catches a mismatched input fingerprint',
              p.returncode == 3, 'exit %d' % p.returncode)

    finally:
        if args.keep:
            print('\nkept %s' % work)
        else:
            shutil.rmtree(work, ignore_errors=True)

    print('\n%s' % ('-' * 60))
    if _fails:
        print('%d of %d checks FAILED:' % (len(_fails), _checks))
        for f in _fails:
            print('  * %s' % f)
        return 1

    print('all %d checks passed' % _checks)
    print('The harness works. What it has NOT done is compare against Max -')
    print('that needs max_out.wav, which only the user can produce (see README).')
    return 0


if __name__ == '__main__':
    sys.exit(main())
