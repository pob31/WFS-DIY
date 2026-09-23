#!/usr/bin/env python3
"""Generate the Effects tab's module-control descriptors and their strings
from Documentation/WFS-UI_effects.csv.

The CSV is the contract for every chain-module control: kind (UI column),
range, default, unit, enum, and - for the reverb - which of its models use
the control (the Models column: empty for all of them, else the model ids,
"1,4,5"). This script emits

  Source/gui/effects/EffectsModuleDescriptors.h   (C++ descriptor tables)
  Resources/lang/en.json                           (effects.labels / help /
                                                    enums / modules / chain)

so the two cannot drift. Edit the CSV, run this from the repo root:

    python tools/gen_effects_module_ui.py

It is idempotent: a second run changes nothing.
"""
import collections
import csv
import io
import json
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
CSV = os.path.join(ROOT, 'Documentation', 'WFS-UI_effects.csv')
HDR = os.path.join(ROOT, 'Source', 'gui', 'effects', 'EffectsModuleDescriptors.h')
LNG = os.path.join(ROOT, 'Resources', 'lang', 'en.json')

SECTIONS = ['FxDist', 'FxEQ', 'FxDyn', 'FxMod', 'FxPhaser', 'FxTrem', 'FxReverb', 'FxDelay', 'FxCrush']
MODULE_NAMES = collections.OrderedDict([
    ('dist', 'Distortion'), ('eq1', 'EQ 1'), ('eq2', 'EQ 2'), ('dyn1', 'Dynamics 1'), ('dyn2', 'Dynamics 2'),
    ('mod', 'Chorus / Flanger'), ('phaser', 'Phaser'), ('trem', 'Tremolo'), ('reverb', 'Reverb'),
    ('delay', 'Multitap Delay'), ('crush', 'Bitcrusher'),
])
KIND = {
    'Text button': 'Toggle', 'H slider': 'Slider', 'H slider (log)': 'LogSlider',
    'H bidirectional slider': 'BiSlider', 'Basic dial': 'Dial', 'Drop down menu': 'Combo',
    'Rotation dial': 'Rotation',
}
CHAIN_STRINGS = collections.OrderedDict([
    ('unlinked', "Unlinked: this chain is this channel's alone."),
    ('linked', 'Linked, {group} ({mode}): the chain order, the bypasses and every module parameter are shared with {count} other channel(s). Ctrl-drag to edit this channel alone.'),
    ('linkedAlone', 'Linked, {group} ({mode}): no other channel is in this group yet.'),
    ('linkedOff', '{group}, link mode OFF: this channel neither sends nor receives chain edits.'),
    ('bypassOn', 'Chain: ON'),
    ('bypassOff', 'Chain: BYPASSED'),
    ('latency', 'Latency: {samples} smp ({ms} ms)'),
    ('latencyIdle', 'Latency: - (processing stopped)'),
    ('reorderHint', 'Drag a module to reorder the chain. Click a module to edit it.'),
    ('moduleOn', 'ON'),
    ('moduleOff', 'OFF'),
    ('tap', 'Tap {n}'),
    ('taps', 'Taps'),
    ('tapTime', 'Time'),
    ('tapLevel', 'Level'),
    ('grMeter', 'GR'),
    ('band', 'Band {n}'),
    ('presetApplied', 'Reverb preset "{name}" applied to this chain.'),
])
EXTRA_HELP = collections.OrderedDict([
    ('chainTile', 'Click to edit this module; drag left or right to move it in the chain. The dot shows whether the module is ON, the bar its output level (gain reduction for Dynamics).'),
    ('chainBypass', "Bypass the whole chain: the feed passes straight to the return with the chain's latency held, so switching back is click-free."),
    ('chainLatency', "The chain's current latency, from the modules that are on (oversampling, lookahead, delay lines). Applied at the next block."),
    ('linkBadge', 'Whether this chain is shared with a link group. Set the group and the link mode on the Channel Parameters tab.'),
    ('eqFlatten', 'Long-press: reset every band of this EQ to its default shape, frequency, gain and Q.'),
    ('eqBandReset', 'Long-press: reset this band to its default shape, frequency, gain and Q.'),
    ('eqBandToggle', "Switch this band on or off. Off keeps the band's settings for when it comes back."),
    ('eqDisplay', 'Drag a band handle to set its frequency and gain; the wheel sets its Q.'),
    ('grMeter', 'Gain reduction the dynamics module is applying right now.'),
    ('tapTime', "This tap's delay time. In Pattern mode the pattern sets it and the slider follows."),
    ('tapLevel', "This tap's level."),
])


def key_of(var):
    """effectDistDrive -> distDrive; effectEQshape -> eqShape; effectEQBypass -> eqBypass"""
    assert var.startswith('effect'), var
    k = var[len('effect'):]
    m = re.match(r'^([A-Z]+)(.*)$', k)
    run, rest = m.group(1), m.group(2)
    if len(run) == 1:
        return run.lower() + rest
    if len(run) >= 3:
        return run[:-1].lower() + run[-1] + rest
    if rest and rest[0].islower():
        return run.lower() + rest[0].upper() + rest[1:]
    return run.lower() + rest


def flit(v):
    s = '%g' % v
    if 'e' in s:
        s = '%f' % v
    if '.' not in s:
        s += '.0'
    return s + 'f'


def slug_of(name):
    name = name.strip()
    if name and name[0].isdigit():
        return 'n' + re.sub(r'[^0-9A-Za-z]+', '', name)
    words = [w for w in re.split(r'[^0-9A-Za-z]+', name) if w]
    return words[0].lower() + ''.join(w[:1].upper() + w[1:] for w in words[1:])


def parse_enum(s):
    """'Auto (0) ; Off (1)' -> [(0,'Auto'), (1,'Off')]; 'OFF ; ON' -> [(0,'OFF'),(1,'ON')]"""
    items = []
    for i, part in enumerate(p.strip() for p in s.split(';')):
        m = re.match(r'^(.*?)\s*\((-?\d+)\)$', part)
        items.append((int(m.group(2)), m.group(1).strip()) if m else (i, part))
    return items


def num(s):
    try:
        return float(s.split('/')[0])
    except ValueError:
        return 0.0


def read_rows():
    with io.open(CSV, encoding='utf-8', newline='') as f:
        rows = list(csv.reader(f, delimiter='\t'))
    head, rows = rows[0], rows[1:]
    col_index = {n: i for i, n in enumerate(head)}

    def col(r, name):
        i = col_index[name]
        return r[i].strip() if i < len(r) else ''

    labels, helps, enums = collections.OrderedDict(), collections.OrderedDict(), collections.OrderedDict()
    modules = collections.OrderedDict((k, []) for k in SECTIONS)
    eq_rows, tap_rows = {}, {}

    for r in rows:
        sec = col(r, 'Section')
        if sec not in modules:
            continue
        var = col(r, 'Variable')
        if var in ('', '(none)'):
            continue
        ui = col(r, 'UI')
        if ui not in KIND:
            raise SystemExit('unknown UI kind %r for %s' % (ui, var))
        label = col(r, 'Label')
        enum = col(r, 'enum')
        key = key_of(var)
        labels[key] = (label.replace(' Bypass', '') if var.endswith('Bypass') else label) + ':'
        helps[key] = col(r, 'Hover help text in the status bar')

        kind = 'Bypass' if var.endswith('Bypass') else KIND[ui]
        items = parse_enum(enum) if enum else []
        if kind == 'Toggle' and not items:
            raise SystemExit('toggle without enum: ' + var)
        enum_prefix = ''
        if items and kind != 'Bypass':
            if var in ('effectModShape', 'effectPhaserShape'):
                enum_prefix = 'effects.lfo.shapes.'          # sine..random live there already
            elif enum == 'OFF ; ON':
                enum_prefix = 'effects.enums.onOff.'
            else:
                enum_prefix = 'effects.enums.%s.' % key
                enums[key] = collections.OrderedDict((slug_of(n), n) for _, n in items)

        models = [int(m) for m in col(r, 'Models').replace(' ', '').split(',') if m]
        if models and sec != 'FxReverb':
            raise SystemExit('a Models cell outside the reverb: ' + var)
        ctrl = dict(var=var, key=key, kind=kind, min=num(col(r, 'Min')), max=num(col(r, 'Max')),
                    default=num(col(r, 'Default')), unit=col(r, 'Unit'), enum_prefix=enum_prefix,
                    items=[(v, slug_of(n)) for v, n in items],
                    mask=sum(1 << m for m in models))
        if sec == 'FxEQ':
            eq_rows[var] = ctrl
        elif var in ('effectDelayTapTime', 'effectDelayTapLevel'):
            tap_rows[var] = ctrl
        else:
            modules[sec].append(ctrl)

    return modules, eq_rows, tap_rows, labels, helps, enums


def emit_ctrl(c):
    items = ', '.join('{ %d, "%s" }' % (v, s) for v, s in c['items'])
    return ('        { WFSParameterIDs::%s, "%s", Kind::%s, %s, %s, %s, "%s", "%s", { %s }, 0x%xu },'
            % (c['var'], c['key'], c['kind'], flit(c['min']), flit(c['max']), flit(c['default']),
               c['unit'], c['enum_prefix'], items, c['mask']))


HEADER_TOP = '''#pragma once

// GENERATED from Documentation/WFS-UI_effects.csv by
// tools/gen_effects_module_ui.py - the CSV is the contract for every module control (kind, range, default,
// unit, enum) and its strings live under effects.labels / effects.help /
// effects.enums in en.json, keyed by the identifier's suffix. Edit the CSV,
// re-run the generator; do not edit this file by hand.

#include <JuceHeader.h>
#include <vector>
#include "../../Parameters/WFSParameterIDs.h"

namespace EffectsUi
{

enum class Kind
{
    Bypass,      // the module's on/off: shown as "<Module>: ON" when the stored bypass is 0
    Toggle,      // two named states from the enum
    Slider,      // linear: min + (max - min) * x
    LogSlider,   // log: min * (max / min)^x
    BiSlider,    // linear, centred: for a symmetric range around 0
    Dial,        // linear, on a small dial
    Combo,       // named values
    Rotation,    // degrees on a rotation dial
};

struct EnumItem { int value; const char* slug; };

struct ControlDesc
{
    const juce::Identifier& id;
    const char* key;            // effects.labels.<key>, effects.help.<key>
    Kind kind;
    float min, max, def;
    const char* unit;
    const char* enumPrefix;     // LOC prefix for the items' slugs ("" when none)
    std::vector<EnumItem> items;
    unsigned modelMask;         // reverb: bit m set = used by model m; 0 = by every model
};

struct ModuleControls { const ControlDesc* controls; int count; };

/** Whether a reverb control is used by `resolvedModel` - the model
    spatcore::effects::resolveReverbModel says a stored id runs. The CSV's
    Models column; every other module's controls answer true. */
inline bool isVisibleForModel (const ControlDesc& d, int resolvedModel)
{
    if (d.modelMask == 0)
        return true;
    return resolvedModel >= 0 && resolvedModel < 32 && ((d.modelMask >> resolvedModel) & 1u) != 0;
}
'''

HEADER_TAIL = '''/** The controls of a declared slot (spatcore::effects::kSlots order). The
    EQ instances return their bypass only - the bands are the EQ panel's. */
inline ModuleControls controlsForSlot (int slot)
{
    switch (slot)
    {
        case 0:  return controlsForDist();
        case 1:
        case 2:  return { &eqBypass(), 1 };
        case 3:
        case 4:  return controlsForDyn();
        case 5:  return controlsForMod();
        case 6:  return controlsForPhaser();
        case 7:  return controlsForTrem();
        case 8:  return controlsForReverb();
        case 9:  return controlsForDelay();
        case 10: return controlsForCrush();
        default: return { nullptr, 0 };
    }
}

} // namespace EffectsUi
'''


def emit_header(modules, eq_rows, tap_rows):
    out = [HEADER_TOP]
    for sec, ctrls in modules.items():
        if sec == 'FxEQ':
            continue
        out.append('inline ModuleControls controlsFor%s()\n{\n    static const ControlDesc list[] = {' % sec[2:])
        out.extend(emit_ctrl(c) for c in ctrls)
        out.append('    };\n    return { list, static_cast<int> (std::size (list)) };\n}\n')

    def single(fn, c):
        out.append('inline const ControlDesc& %s()\n{\n    static const ControlDesc d = ' % fn)
        out.append(emit_ctrl(c).strip().rstrip(',') + ';\n    return d;\n}\n')

    single('eqBypass', eq_rows['effectEQBypass'])
    for var in ('effectEQshape', 'effectEQfreq', 'effectEQgain', 'effectEQq', 'effectEQslope',
                'effectDelayTapTime', 'effectDelayTapLevel'):
        single('desc' + var[len('effect'):], eq_rows.get(var) or tap_rows[var])
    out.append(HEADER_TAIL)
    return '\n'.join(out)


def write_if_changed(path, text, newline):
    old = io.open(path, encoding='utf-8', newline='').read() if os.path.exists(path) else None
    if old == text:
        return False
    with io.open(path, 'w', encoding='utf-8', newline=newline) as f:
        f.write(text)
    return True


def update_strings(labels, helps, enums):
    raw = io.open(LNG, encoding='utf-8', newline='').read()
    nl = '\r\n' if '\r\n' in raw else '\n'
    d = json.loads(raw, object_pairs_hook=collections.OrderedDict)
    e = d['effects']

    e['modules'] = collections.OrderedDict(MODULE_NAMES)
    for k, v in labels.items():
        e['labels'][k] = v
    for k, v in helps.items():
        e['help'][k] = v
    for k, v in EXTRA_HELP.items():
        e['help'][k] = v
    e['enums'] = collections.OrderedDict([('onOff', collections.OrderedDict([('off', 'OFF'), ('on', 'ON')]))])
    for k, v in enums.items():
        if k != 'onOff':
            e['enums'][k] = v
    e['chain'] = collections.OrderedDict(CHAIN_STRINGS)

    # Splice the effects block back in place, so the rest of the file is untouched.
    s0 = raw.index(nl + '  "effects": {')
    s1 = raw.index(nl + '  "inputs": {', s0)
    block = json.dumps(e, ensure_ascii=True, indent=2)
    block = nl.join(('  ' + ln) if ln.strip() else ln for ln in block.split('\n'))
    new = raw[:s0] + nl + '  "effects": ' + block.lstrip() + ',' + raw[s1:]
    return write_if_changed(LNG, new, '')


def main():
    modules, eq_rows, tap_rows, labels, helps, enums = read_rows()
    changed_h = write_if_changed(HDR, emit_header(modules, eq_rows, tap_rows), '\n')
    changed_j = update_strings(labels, helps, enums)
    n = sum(len(v) for v in modules.values()) + len(eq_rows) + len(tap_rows)
    print('effects module UI: %d controls; header %s, strings %s'
          % (n, 'written' if changed_h else 'unchanged', 'written' if changed_j else 'unchanged'))
    return 0


if __name__ == '__main__':
    sys.exit(main())
