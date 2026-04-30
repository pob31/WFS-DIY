"""Layer 1: hand-crafted nasty OSC packets.

Each entry is a Case dataclass. The fuzzer iterates the list, sends the
encoded packet, then runs detection (PID liveness, log tail, optional
read-back).

Fields:
  category:    one-word group name, used to bucket findings
  address:     OSC address pattern
  args:        list of (tag, value) pairs for the OSC codec. Use 'f' for
               float32, 'i' for int32, 's' for string, 'X' for raw bytes,
               etc. Mismatched tags are intentional.
  expect:      'ignored' / 'clamped' / 'may-crash' / 'may-leak-nan' /
               'silent-wrong' — what we hope happens. Used as documentation
               only; the fuzzer reports actual outcome regardless.
  notes:       short human-readable description.

To add a case: append to CASES. Order doesn't matter, but cases sharing a
category are easier to bucket in findings/summary.csv.
"""

from __future__ import annotations

import math
import struct
from dataclasses import dataclass, field
from typing import Any


NAN = float("nan")
PINF = float("inf")
NINF = float("-inf")
INT_MIN = -(2**31)
INT_MAX = 2**31 - 1
FLT_DENORM = 1e-40
FLT_HUGE = 1e30
FLT_TINY = 1e-30
BIG_STRING = "A" * 60000  # below UDP datagram limit (~65507B)
DEEP_PATH = "/wfs/input/" + "/".join(["x"] * 200)


@dataclass
class Case:
    category: str
    address: str
    args: list[tuple[str, Any]] = field(default_factory=list)
    expect: str = "ignored"
    notes: str = ""


# Channel ID used for "valid"-shape packets. Should be a real channel; the
# fuzzer takes --channel from the CLI but corpus uses 1 by default.
CH = 1


# ---------------------------------------------------------------------------
# Layer 1 corpus
# ---------------------------------------------------------------------------

CASES: list[Case] = [
    # -----------------------------------------------------------------------
    # 1) NaN / Inf into position params (jlimit-clamped path)
    # -----------------------------------------------------------------------
    Case("nan-position", "/wfs/input/positionX",
         [("i", CH), ("f", NAN)],
         expect="may-leak-nan",
         notes="NaN through applyConstraintX (juce::jlimit returns NaN)"),
    Case("nan-position", "/wfs/input/positionY",
         [("i", CH), ("f", NAN)], expect="may-leak-nan"),
    Case("nan-position", "/wfs/input/positionZ",
         [("i", CH), ("f", NAN)], expect="may-leak-nan"),
    Case("inf-position", "/wfs/input/positionX",
         [("i", CH), ("f", PINF)], expect="may-leak-nan"),
    Case("inf-position", "/wfs/input/positionY",
         [("i", CH), ("f", NINF)], expect="may-leak-nan"),
    Case("inf-position", "/wfs/input/offsetX",
         [("i", CH), ("f", PINF)], expect="may-leak-nan"),

    # -----------------------------------------------------------------------
    # 2) NaN / Inf / huge into non-position params (no clamp suspected)
    # -----------------------------------------------------------------------
    Case("nan-nonpos", "/wfs/input/attenuation",
         [("i", CH), ("f", NAN)], expect="may-leak-nan",
         notes="non-position: handleRemoteParameterSet sets value w/o clamp"),
    Case("inf-nonpos", "/wfs/input/attenuation",
         [("i", CH), ("f", PINF)], expect="may-leak-nan"),
    Case("huge-nonpos", "/wfs/input/attenuation",
         [("i", CH), ("f", FLT_HUGE)], expect="silent-wrong"),
    Case("huge-nonpos", "/wfs/input/reverbSend",
         [("i", CH), ("f", -FLT_HUGE)], expect="silent-wrong"),
    Case("huge-nonpos", "/wfs/input/LSradius",
         [("i", CH), ("f", FLT_HUGE)], expect="silent-wrong"),
    Case("huge-nonpos", "/wfs/input/jitter",
         [("i", CH), ("f", FLT_HUGE)], expect="silent-wrong"),
    Case("nan-nonpos", "/wfs/input/LFOperiod",
         [("i", CH), ("f", NAN)], expect="may-leak-nan",
         notes="LFO period feeds the LFO clock; NaN could stick the phase"),
    Case("zero-nonpos", "/wfs/input/LFOperiod",
         [("i", CH), ("f", 0.0)], expect="silent-wrong",
         notes="zero period — division by zero candidate in LFO update"),
    Case("denorm", "/wfs/input/LFOamplitudeX",
         [("i", CH), ("f", FLT_DENORM)], expect="ignored",
         notes="subnormal float — DSP perf/precision concern"),

    # -----------------------------------------------------------------------
    # 3) Channel-ID extremes
    # -----------------------------------------------------------------------
    Case("ch-id", "/wfs/input/positionX",
         [("i", -1), ("f", 0.5)], expect="ignored",
         notes="negative channel — getInputParameter must bounds-check"),
    Case("ch-id", "/wfs/input/positionX",
         [("i", 0), ("f", 0.5)], expect="ignored",
         notes="channel 0 (1-based protocol)"),
    Case("ch-id", "/wfs/input/positionX",
         [("i", 99999), ("f", 0.5)], expect="ignored"),
    Case("ch-id", "/wfs/input/positionX",
         [("i", INT_MAX), ("f", 0.5)], expect="ignored"),
    Case("ch-id", "/wfs/input/positionX",
         [("i", INT_MIN), ("f", 0.5)], expect="ignored"),

    # -----------------------------------------------------------------------
    # 4) Cluster ID extremes
    # -----------------------------------------------------------------------
    Case("cluster-id", "/cluster/move",
         [("i", -1), ("f", 1.0), ("f", 1.0)], expect="ignored"),
    Case("cluster-id", "/cluster/move",
         [("i", 99999), ("f", 1.0), ("f", 1.0)], expect="ignored"),
    Case("cluster-id", "/cluster/scale",
         [("i", -1), ("f", 2.0)], expect="ignored"),
    Case("cluster-id", "/cluster/scaleRotation",
         [("i", INT_MAX), ("f", 1.0), ("f", 0.0)], expect="ignored"),
    Case("cluster-nan", "/cluster/move",
         [("i", 1), ("f", NAN), ("f", NAN)], expect="may-leak-nan"),
    Case("cluster-nan", "/cluster/scale",
         [("i", 1), ("f", PINF)], expect="may-leak-nan"),

    # -----------------------------------------------------------------------
    # 5) Type confusion
    # -----------------------------------------------------------------------
    Case("type-confusion", "/wfs/input/positionX",
         [("i", CH), ("s", "hello")], expect="silent-wrong",
         notes="string where float expected — extractFloat returns 0.0"),
    Case("type-confusion", "/wfs/input/positionX",
         [("s", "1"), ("f", 0.5)], expect="silent-wrong",
         notes="string where int channel expected"),
    Case("type-confusion", "/wfs/input/attenuation",
         [("i", CH), ("i", 1)], expect="silent-wrong",
         notes="int where float expected"),
    Case("type-confusion", "/wfs/input/attenuation",
         [("i", CH), ("d", 0.5)], expect="silent-wrong",
         notes="float64 where float32 expected"),
    Case("type-confusion", "/wfs/input/positionX",
         [("i", CH), ("f", 0.5), ("T", True)], expect="ignored",
         notes="bool tag mid-stream — extra trailing arg"),
    Case("type-confusion", "/wfs/input/attenuation",
         [("i", CH), ("b", b"\xff" * 32)], expect="silent-wrong",
         notes="blob where float expected"),

    # -----------------------------------------------------------------------
    # 6) Truncated / oversized argument lists
    # -----------------------------------------------------------------------
    Case("truncated", "/wfs/input/positionX", [], expect="ignored",
         notes="no args at all — must early-out"),
    Case("truncated", "/wfs/input/positionX",
         [("i", CH)], expect="ignored",
         notes="channel only, no value — size()<2 guard"),
    Case("oversized", "/wfs/input/positionX",
         [("i", CH), ("f", 0.5), ("f", 0.0), ("f", 0.0), ("f", 0.0),
          ("f", 0.0), ("f", 0.0), ("f", 0.0), ("f", 0.0)], expect="ignored",
         notes="9 args where 2-3 expected"),

    # -----------------------------------------------------------------------
    # 7) Oversized strings
    # -----------------------------------------------------------------------
    Case("huge-string", "/wfs/input/name",
         [("i", CH), ("s", BIG_STRING)], expect="ignored",
         notes="64KiB name — does ValueTree set choke?"),
    Case("huge-string", "/wfs/input/positionX",
         [("i", CH), ("s", BIG_STRING)], expect="silent-wrong",
         notes="64KiB string where float expected"),

    # -----------------------------------------------------------------------
    # 8) Address-routing variants
    # -----------------------------------------------------------------------
    Case("addr-typo", "/wfs/inpt/positionX",
         [("i", CH), ("f", 0.5)], expect="ignored",
         notes="typo in prefix"),
    Case("addr-case", "/wfs/INPUT/positionX",
         [("i", CH), ("f", 0.5)], expect="ignored",
         notes="upper-case prefix"),
    Case("addr-case", "/wfs/input/PositionX",
         [("i", CH), ("f", 0.5)], expect="ignored",
         notes="param-name case mismatch"),
    Case("addr-double-slash", "/wfs/input//positionX",
         [("i", CH), ("f", 0.5)], expect="ignored"),
    Case("addr-trailing-slash", "/wfs/input/positionX/",
         [("i", CH), ("f", 0.5)], expect="ignored"),
    Case("addr-empty", "",
         [("f", 0.5)], expect="ignored"),
    Case("addr-deep", DEEP_PATH,
         [("i", CH), ("f", 0.5)], expect="ignored",
         notes="200-segment path — parser depth"),
    Case("addr-unknown", "/wfs/input/totallyMadeUpParam",
         [("i", CH), ("f", 0.5)], expect="ignored"),
    Case("addr-adm", "/adm/obj/-1/xyz",
         [("f", 0.0), ("f", 0.0), ("f", 0.0)], expect="ignored",
         notes="negative ADM object ID"),
    Case("addr-adm", "/adm/obj/99999/xyz",
         [("f", NAN), ("f", PINF), ("f", NINF)], expect="may-leak-nan"),

    # -----------------------------------------------------------------------
    # 9) Config/global params (no channel ID)
    # -----------------------------------------------------------------------
    Case("config-nan", "/wfs/config/stage/width",
         [("f", NAN)], expect="may-leak-nan",
         notes="stage geometry NaN — would propagate to renderer"),
    Case("config-nan", "/wfs/config/stage/height",
         [("f", PINF)], expect="may-leak-nan"),
    Case("config-huge", "/wfs/config/stage/width",
         [("f", FLT_HUGE)], expect="silent-wrong"),
    Case("config-zero", "/wfs/config/stage/width",
         [("f", 0.0)], expect="silent-wrong",
         notes="zero stage width — divide-by-zero in normalization?"),
    Case("config-neg", "/wfs/config/stage/width",
         [("f", -10.0)], expect="silent-wrong",
         notes="negative dimension"),
    Case("config-rt60", "/wfs/config/reverb/rt60",
         [("f", NAN)], expect="may-leak-nan",
         notes="NaN RT60 feeds reverb decay — DSP-side blowup risk"),
    Case("config-rt60", "/wfs/config/reverb/rt60",
         [("f", FLT_HUGE)], expect="silent-wrong"),

    # -----------------------------------------------------------------------
    # 10) Reverb per-channel
    # -----------------------------------------------------------------------
    Case("reverb-nan", "/wfs/reverb/positionX",
         [("i", CH), ("f", NAN)], expect="may-leak-nan"),
    Case("reverb-nan", "/wfs/reverb/preEQfreq",
         [("i", CH), ("f", NAN)], expect="may-leak-nan",
         notes="EQ frequency NaN — biquad coefficient blowup"),
    Case("reverb-zero-freq", "/wfs/reverb/preEQfreq",
         [("i", CH), ("f", 0.0)], expect="silent-wrong",
         notes="0 Hz EQ — log() in coefficient calc"),
    Case("reverb-neg-freq", "/wfs/reverb/preEQfreq",
         [("i", CH), ("f", -1000.0)], expect="silent-wrong"),
    Case("reverb-huge-q", "/wfs/reverb/preEQq",
         [("i", CH), ("f", FLT_HUGE)], expect="silent-wrong"),

    # -----------------------------------------------------------------------
    # 11) Output per-channel
    # -----------------------------------------------------------------------
    Case("output-nan", "/wfs/output/positionX",
         [("i", CH), ("f", NAN)], expect="may-leak-nan"),
    Case("output-nan", "/wfs/output/EQfreq",
         [("i", CH), ("f", NAN)], expect="may-leak-nan"),
    Case("output-nan", "/wfs/output/orientation",
         [("i", CH), ("f", NAN)], expect="may-leak-nan"),
    Case("output-huge", "/wfs/output/HFdamping",
         [("i", CH), ("f", FLT_HUGE)], expect="silent-wrong"),

    # -----------------------------------------------------------------------
    # 12) /remoteInput (direct ValueTree write path)
    # -----------------------------------------------------------------------
    Case("remote-nan", "/remoteInput/positionX",
         [("i", CH), ("f", NAN)], expect="may-leak-nan"),
    Case("remote-nan", "/remoteInput/attenuation",
         [("i", CH), ("f", PINF)], expect="may-leak-nan",
         notes="OSCManager.cpp:2241 — non-position written w/o clamp"),
    Case("remote-huge", "/remoteInput/maxSpeed",
         [("i", CH), ("f", FLT_HUGE)], expect="silent-wrong"),

    # -----------------------------------------------------------------------
    # 13) Raw-bytes garbage at the OSC boundary
    # -----------------------------------------------------------------------
    Case("raw-truncated", "/wfs/input/positionX",
         [("X", b"")], expect="ignored",
         notes="no type tag string at all — just address"),
    Case("raw-truncated", "/wfs/input/positionX",
         [("X", b",f\x00\x00\x00")], expect="ignored",
         notes="says 'f' but only 1 byte of payload"),
    Case("raw-bad-tag", "/wfs/input/positionX",
         [("X", b",zzzz\x00\x00\x00\x00\x00\x00\x00\x00")], expect="ignored",
         notes="undefined tag chars"),
]


# Synthetic raw-bytes cases that bypass the codec entirely, sent verbatim.
# Format: (category, raw_bytes, expect, notes)
RAW_PACKETS: list[tuple[str, bytes, str, str]] = [
    ("raw-malformed", b"", "ignored", "empty datagram"),
    ("raw-malformed", b"not an osc packet", "ignored",
     "no leading slash, no nulls"),
    ("raw-malformed", b"/wfs/input/positionX", "ignored",
     "address only, no type-tag terminator"),
    ("raw-malformed", b"/wfs/input/positionX\x00\x00\x00\x00", "ignored",
     "address ok, no tag string"),
    ("raw-malformed",
     b"/wfs/input/positionX\x00\x00\x00\x00,fff\x00\x00\x00\x00", "ignored",
     "claims 3 floats but no payload"),
    ("raw-malformed", b"\x00" * 1024, "ignored", "all zero bytes"),
    ("raw-malformed", b"\xff" * 1024, "ignored", "all 0xff bytes"),
    ("raw-malformed", struct.pack(">i", -1) + b"#bundle\x00", "ignored",
     "garbage bundle header"),
]
