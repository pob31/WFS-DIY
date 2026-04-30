"""Layer 2: curated address inventory and per-address value generators.

Each entry describes one address family. The fuzzer iterates entries, then
for each entry runs the value generator over a fixed pool of edge-case
inputs, encoding each as a packet against the address.

This is a hand-curated subset of the maps in OSCMessageRouter.cpp — one
representative per parameter family. We do not want to hammer every
permutation; we want enough breadth to catch family-level bugs.

Fields:
  address:     OSC address pattern
  shape:       'channel-float' (i, f) | 'channel-string' (i, s) |
               'channel-float-float-float' (i, f, f, f) |
               'config-float' (f) | 'cluster-move' (i, f, f) |
               'cluster-scalar' (i, f) | 'channel-int' (i, i) |
               'channel-int-int' (i, i, i)
  value_kind:  'pos' | 'norm' | 'freq' | 'gain' | 'time' | 'count' |
               'bool' | 'angle' | 'name' | 'free' — guides the generator.
  read_back:   optional OSCQuery path to GET ?VALUE after writes, e.g.
               '/wfs/input/{ch}/positionX'. None disables read-back.
"""

from __future__ import annotations

import math
from dataclasses import dataclass
from typing import Callable, Iterable, Optional


@dataclass
class AddrEntry:
    address: str
    shape: str
    value_kind: str
    read_back: Optional[str] = None
    notes: str = ""


# Channel used for tests that target a per-channel param.
CH = 1


# Curated representative addresses — ~30 entries
ENTRIES: list[AddrEntry] = [
    # Position constraint path (jlimit-based, NaN-leaky)
    AddrEntry("/wfs/input/positionX", "channel-float", "pos",
              read_back="/wfs/input/{ch}/positionX",
              notes="constrained, but jlimit propagates NaN"),
    AddrEntry("/wfs/input/positionY", "channel-float", "pos",
              read_back="/wfs/input/{ch}/positionY"),
    AddrEntry("/wfs/input/positionZ", "channel-float", "pos",
              read_back="/wfs/input/{ch}/positionZ"),
    AddrEntry("/wfs/input/offsetX", "channel-float", "pos",
              read_back="/wfs/input/{ch}/offsetX"),

    # Per-channel float params w/o explicit clamp
    AddrEntry("/wfs/input/attenuation", "channel-float", "norm",
              read_back="/wfs/input/{ch}/attenuation",
              notes="no clamp at OSCManager.cpp:2241"),
    AddrEntry("/wfs/input/delayLatency", "channel-float", "time",
              read_back="/wfs/input/{ch}/delayLatency"),
    AddrEntry("/wfs/input/reverbSend", "channel-float", "norm",
              read_back="/wfs/input/{ch}/reverbSend"),
    AddrEntry("/wfs/input/jitter", "channel-float", "norm",
              read_back="/wfs/input/{ch}/jitter"),

    # LFO family
    AddrEntry("/wfs/input/LFOperiod", "channel-float", "time",
              read_back="/wfs/input/{ch}/LFOperiod",
              notes="zero/NaN period — clock division risk"),
    AddrEntry("/wfs/input/LFOamplitudeX", "channel-float", "norm",
              read_back="/wfs/input/{ch}/LFOamplitudeX"),
    AddrEntry("/wfs/input/LFOphaseX", "channel-float", "angle",
              read_back="/wfs/input/{ch}/LFOphaseX"),

    # LSactive family
    AddrEntry("/wfs/input/LSradius", "channel-float", "pos",
              read_back="/wfs/input/{ch}/LSradius"),
    AddrEntry("/wfs/input/LSpeakThreshold", "channel-float", "gain",
              read_back="/wfs/input/{ch}/LSpeakThreshold"),

    # FR family
    AddrEntry("/wfs/input/FRlowCutFreq", "channel-float", "freq",
              read_back="/wfs/input/{ch}/FRlowCutFreq",
              notes="0 Hz / negative freq → log() in biquad"),
    AddrEntry("/wfs/input/FRhighShelfGain", "channel-float", "gain",
              read_back="/wfs/input/{ch}/FRhighShelfGain"),

    # AutomOtion
    AddrEntry("/wfs/input/otomoX", "channel-float", "pos",
              read_back="/wfs/input/{ch}/otomoX"),
    AddrEntry("/wfs/input/otomoDuration", "channel-float", "time",
              read_back="/wfs/input/{ch}/otomoDuration"),
    AddrEntry("/wfs/input/otomoR", "channel-float", "pos",
              read_back="/wfs/input/{ch}/otomoR"),

    # Strings
    AddrEntry("/wfs/input/name", "channel-string", "name",
              read_back="/wfs/input/{ch}/name"),

    # Bool/int
    AddrEntry("/wfs/input/LFOenable", "channel-int", "bool",
              read_back="/wfs/input/{ch}/LFOactive"),
    AddrEntry("/wfs/input/cluster", "channel-int", "count",
              read_back="/wfs/input/{ch}/cluster"),
    AddrEntry("/wfs/input/trackingID", "channel-int", "count",
              read_back="/wfs/input/{ch}/trackingID"),

    # Output
    AddrEntry("/wfs/output/positionX", "channel-float", "pos",
              read_back="/wfs/output/{ch}/positionX"),
    AddrEntry("/wfs/output/orientation", "channel-float", "angle",
              read_back="/wfs/output/{ch}/orientation"),
    AddrEntry("/wfs/output/EQfreq", "channel-float", "freq",
              read_back="/wfs/output/{ch}/EQfreq"),
    AddrEntry("/wfs/output/EQgain", "channel-float", "gain",
              read_back="/wfs/output/{ch}/EQgain"),

    # Reverb per-channel
    AddrEntry("/wfs/reverb/positionX", "channel-float", "pos",
              read_back="/wfs/reverb/{ch}/positionX"),
    AddrEntry("/wfs/reverb/preEQfreq", "channel-float", "freq",
              read_back="/wfs/reverb/{ch}/preEQfreq"),
    AddrEntry("/wfs/reverb/preEQq", "channel-float", "gain",
              read_back="/wfs/reverb/{ch}/preEQq"),

    # Config (global)
    AddrEntry("/wfs/config/stage/width", "config-float", "pos",
              read_back="/wfs/config/stage/width"),
    AddrEntry("/wfs/config/stage/height", "config-float", "pos",
              read_back="/wfs/config/stage/height"),
    AddrEntry("/wfs/config/reverb/rt60", "config-float", "time",
              read_back="/wfs/config/reverb/rt60"),
    AddrEntry("/wfs/config/reverb/wetLevel", "config-float", "norm",
              read_back="/wfs/config/reverb/wetLevel"),

    # Cluster (special shapes)
    AddrEntry("/cluster/move", "cluster-move", "pos"),
    AddrEntry("/cluster/scale", "cluster-scalar", "norm"),
]


# ---------------------------------------------------------------------------
# Edge-case value pools per kind
# ---------------------------------------------------------------------------

NAN = float("nan")
PINF = float("inf")
NINF = float("-inf")
INT_MIN = -(2**31)
INT_MAX = 2**31 - 1

# Float pool — covers every IEEE-754 corner we care about.
FLOAT_EDGES: list[float] = [
    NAN, PINF, NINF,
    0.0, -0.0,
    1e-40, -1e-40,           # subnormals
    1e-30, 1e30, -1e30,
    3.4028235e38,            # FLT_MAX-ish
    -3.4028235e38,
    1.0, -1.0, 0.5, -0.5,    # benign baseline
]

# Int pool
INT_EDGES: list[int] = [
    INT_MIN, INT_MIN + 1,
    -1, 0, 1,
    INT_MAX - 1, INT_MAX,
    255, 65535, 1_000_000,
]

# String pool — very short, very long, control bytes, unicode.
STRING_EDGES: list[str] = [
    "",
    " ",
    "A",
    "A" * 255,
    "A" * 60000,  # below UDP datagram limit (~65507B)
    "\x00\x01\x02\x03",
    "‮​",  # bidi-override + zero-width + bell
    "../../etc/passwd",
    "🔥" * 256,
]


def floats_for_kind(kind: str) -> list[float]:
    """Edge-case pool, optionally augmented with kind-relevant boundaries."""
    base = list(FLOAT_EDGES)
    if kind == "norm":
        base += [-0.001, 1.001, 2.0, -1.0]
    elif kind == "freq":
        base += [-100.0, 0.0, 1.0, 22050.0, 96000.0, 1e9]
    elif kind == "gain":
        base += [-200.0, 200.0, 1e6]
    elif kind == "time":
        base += [-0.01, 0.0, 0.001, 1e6]
    elif kind == "angle":
        base += [-720.0, 720.0, 360.0, 180.0, -180.0]
    elif kind == "pos":
        base += [-1000.0, 1000.0]
    return base


def ints_for_kind(kind: str) -> list[int]:
    base = list(INT_EDGES)
    if kind == "bool":
        base += [2, -2, 256]
    return base
