"""OSC replay driver.

Launches WFS-DIY on a temp copy of the golden fixture, sends a scripted,
deterministic OSC write sequence over UDP 8000 covering the address
families (per-channel float/string/int writes, config-global, cluster
move, /remoteInput absolute + inc delta, one polar write, one out-of-range
write that must be rejected keep-current, the per-output mute list in its
whole-list, one-output, refused-scalar and back-to-back burst forms), then
reads every touched value
back over OSCQuery HTTP (`GET /<path>?VALUE`) and compares the result set
against a committed golden JSON.

THE EFFECTS FAMILY IS READ BACK FROM THE SAVED PROJECT, not over OSCQuery:
its outbound half and its OSCQuery container are the NEXT commit, so the
namespace publishes no /wfs/effect node yet. The driver calls `session_save`
over MCP and asserts on `effects.xml` instead, which is a stronger read for
this commit anyway: it names the NODE a value landed on (FxEq2 vs FxEq1,
Tap 5 vs Tap 1, column 3 of a row vs column 0), and "reached instance 1 when
it named instance 2" is the failure this surface must not ship - it passes
every "did it write something" check.

The fixture carries `effectChannels="0"`. The driver rewrites that to 2 in
the TEMP COPY before the app opens it (WFSFileManager::applyConfigSection
builds the family from <IO>/effectChannels), deliberately NOT through the
new /wfs/config/effectChannels route: a gate that sets itself up through the
mechanism it is testing cannot fail.

All write values are chosen binary-exact (x.25 / x.5) so float32 round-trip
is bit-stable.

Usage:
  python osc_replay.py [--exe path] [--update] [--keep-temp]

Exit codes: 0 pass, 1 mismatch, 2 usage, 3 app failed to start.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import sys
import time
import xml.etree.ElementTree as ET
from pathlib import Path

import common

GOLDEN = common.GOLDENS_DIR / "osc_replay.json"

# How many effect channels the temp fixture is rewritten to carry.
EFFECT_CHANNELS = 2

# (label, address, [(tag, value), ...])
WRITES = [
    # per-channel float / string / int families
    ("input1.positionX",   "/wfs/input/positionX",   [("i", 1), ("f", -3.25)]),
    ("input2.attenuation", "/wfs/input/attenuation", [("i", 2), ("f", -12.5)]),
    ("input3.name",        "/wfs/input/name",        [("i", 3), ("s", "OSC Renamed")]),
    ("input4.cluster",     "/wfs/input/cluster",     [("i", 4), ("i", 1)]),
    ("input5.colour",      "/wfs/input/colour",      [("i", 5), ("i", 3435973)]),
    ("output1.positionX",  "/wfs/output/positionX",  [("i", 1), ("f", -4.5)]),
    ("reverb1.positionX",  "/wfs/reverb/positionX",  [("i", 1), ("f", 2.25)]),
    # Reverb pre-processing EQ, in BOTH wire forms - they take different arms
    # of parseReverbMessage and each used to be broken in its own way. The
    # standard form stored the band index as the value (its EQ test looked for
    # names beginning "EQ" when every reverb name begins "preEQ"); the
    # OSCQuery form parsed correctly and was then dropped by a band lookup
    # that asked for a child of type "Band2" when bands are <Band id="2">.
    # Bands are 1-based on the wire, as they already are in the MCP tools.
    ("reverb1.preEQgain",  "/wfs/reverb/preEQgain",   [("i", 1), ("i", 2), ("f", -6.5)]),
    ("reverb1.preEQfreq",  "/wfs/reverb/1/preEQfreq", [("i", 3), ("f", 250.0)]),
    # config-global family
    ("stage.width",        "/wfs/config/stage/width", [("f", 14.0)]),
    # cluster family: input 4 joined cluster 1 above; delta-move the cluster
    ("cluster1.move",      "/cluster/move",          [("i", 1), ("f", 0.5), ("f", 0.25)]),
    # remoteInput family: absolute write then inc-delta on the same value
    ("remote5.positionX",  "/remoteInput/positionX", [("i", 5), ("f", 1.75)]),
    # polar write (converted to cartesian internally)
    ("input6.positionR",   "/wfs/input/positionR",   [("i", 6), ("f", 2.0)]),
    # out-of-range write — must be rejected keep-current (bounds are
    # [-50, 50] for inputPositionX, see OSCParameterBounds)
    ("input1.positionX.oob", "/wfs/input/positionX", [("i", 1), ("f", 500.0)]),
    # inc/dec delta over OSC (/remoteInput/<param> <id> "inc" <delta>)
    ("remote5.positionX.inc", "/remoteInput/positionX",
     [("i", 5), ("s", "inc"), ("f", 0.25)]),
    # per-output mute list: a whole (short) list, then one output, then a
    # lone number - which must be refused, not replace the list (it used to
    # unmute every output but the first)
    ("input7.mutes.list",   "/wfs/input/mutes", [("i", 7), ("s", "0,1,0,0,1")]),
    ("input7.mutes.one",    "/wfs/input/mutes", [("i", 7), ("i", 3), ("i", 1)]),
    ("input7.mutes.scalar", "/wfs/input/mutes", [("i", 7), ("i", 0)]),
]

# ---------------------------------------------------------------------------
# The effects family. EIGHT argument shapes over one address prefix, and the
# parser has to know which one arrived from the parameter NAME alone - nothing
# in the argument list distinguishes `<ID> <instance> <value>` from
# `<ID> <value> <fadeSeconds>`. Every shape is exercised here, and every
# assertion below names the NODE as well as the value.
# ---------------------------------------------------------------------------
EFFECT_WRITES = [
    # -- the ordinary per-channel scalar, in BOTH wire forms
    ("fx1.attenuation",  "/wfs/effect/attenuation",  [("i", 1), ("f", -12.5)]),
    ("fx2.positionX",    "/wfs/effect/2/positionX",  [("f", -3.25)]),

    # -- instanced: FxDyn1 and FxDyn2 carry the SAME property name, so a
    #    parser that dropped the instance would put both of these on FxDyn1
    #    and the second would look like a successful write.
    ("fx1.dyn1.threshold", "/wfs/effect/dynCompThreshold", [("i", 1), ("i", 1), ("f", -18.5)]),
    ("fx1.dyn2.threshold", "/wfs/effect/dynCompThreshold", [("i", 1), ("i", 2), ("f", -30.25)]),

    # -- band: instance AND band, four arguments. Same trap, one level deeper.
    ("fx1.eq1.band1.gain", "/wfs/effect/EQgain", [("i", 1), ("i", 1), ("i", 1), ("f", 4.5)]),
    ("fx1.eq2.band3.gain", "/wfs/effect/EQgain", [("i", 1), ("i", 2), ("i", 3), ("f", -6.5)]),

    # -- delay tap (there are eight; tap 5 is not tap 1)
    ("fx1.tap5.time",    "/wfs/effect/delayTapTime", [("i", 1), ("i", 5), ("f", 250.0)]),

    # -- send CELLS, in both keyings. The input-keyed column is an input
    #    PERMANENT number; the effect-keyed column is a dense effect id.
    ("fx1.sendLevel.in3", "/wfs/effect/sendLevel", [("i", 1), ("i", 3), ("f", -24.5)]),
    ("fx1.sendOn.in3",    "/wfs/effect/sendOn",    [("i", 1), ("i", 3), ("i", 1)]),
    ("fx2.fxSendLevel.fx1", "/wfs/effect/fxSendLevel", [("i", 2), ("i", 1), ("f", -12.25)]),
    ("fx2.fxSendOn.fx1",    "/wfs/effect/fxSendOn",    [("i", 2), ("i", 1), ("i", 1)]),
    #    THE DIAGONAL, ADDRESSED ON PURPOSE. An effect may not feed itself, and
    #    setEffectFxSendOnFromEffect is what refuses it. Without this write the
    #    `fx2.fxSendOn.self` assertion below reads a cell no message ever named,
    #    so it asserts a DEFAULT and would pass just as happily with the guard
    #    deleted - an assertion that cannot fail is not a gate.
    ("fx2.fxSendOn.self",   "/wfs/effect/fxSendOn",    [("i", 2), ("i", 2), ("i", 1)]),

    # -- packed ROWS as CSV strings: a switch row (normalised and padded to the
    #    fixed width by the write interceptor) and the chain permutation (kept
    #    verbatim - it is not a send row).
    ("fx2.sendOns.row",  "/wfs/effect/sendOns",   [("i", 2), ("s", "0,1,0,1")]),
    ("fx1.chainOrder",   "/wfs/effect/chainOrder",
     [("i", 1), ("s", "eq1,dist,eq2,dyn1,dyn2,mod,phaser,trem,reverb,delay,crush")]),

    # -- REFUSALS, each of which must leave the stored value exactly as it was.
    #    An out-of-range cell column (the send rows are 64 wide):
    ("fx1.sendLevel.oob", "/wfs/effect/sendLevel", [("i", 1), ("i", 99), ("f", -6.0)]),
    #    A bare scalar sent at a ROW identifier. This is the write that used to
    #    replace a whole routing with one token:
    ("fx2.sendOns.scalar", "/wfs/effect/sendOns", [("i", 2), ("i", 0)]),
    #    A non-numeric string sent at a numeric scalar:
    ("fx1.attenuation.text", "/wfs/effect/attenuation", [("i", 1), ("s", "loud")]),
    #    An out-of-range EQ instance (there are two):
    ("fx1.eq3.band1.gain", "/wfs/effect/EQgain", [("i", 1), ("i", 3), ("i", 1), ("f", -20.0)]),

    # -- the GLOBALS, which take <value> and NO effect id. Both spellings the
    #    published contract uses: the nine under /wfs/config/effects/ and the
    #    one the contract keeps under the channel prefix although the property
    #    is a Config one. Both must land in the EFFECTS undo domain, not the
    #    reverb one the /wfs/config/ branch used to be hard-wired to.
    ("global.linkMode",     "/wfs/config/effects/linkMode",        [("i", 2)]),
    ("global.loopGuardCeil", "/wfs/config/effects/loopGuardCeiling", [("f", 3.5)]),
    ("global.mapVisible",   "/wfs/effect/mapVisible",              [("i", 0)]),
    #    ...and a global sent WITH an effect id is refused, not written to
    #    channel 1 (the reverse of the shape test above).
    ("global.linkMode.withId", "/wfs/effect/1/mapVisible",         [("i", 1)]),
]

# THE BURST C6 EXISTS FOR. 64 cell writes to ONE effect in one datagram train.
# The ingest key is `address | first int32` and the first int32 of a cell
# message is the EFFECT, so without /wfs/effect/sendLevel in the classifier's
# bypass list one cell per 16 ms drain tick survives; without the sub-index in
# OSCManager's own coalesce key, exactly one survives full stop.
CELL_BURST = [("/wfs/effect/sendLevel", [("i", 2), ("i", k), ("f", float(-k))])
              for k in range(1, 65)]

# Sent LAST, after every assertion above has had its value written: an accepted
# channel-count write. Structural, so it is refused while processing runs - the
# other half of that gate is RUNNING_REFUSAL below.
COUNT_WRITE = ("/wfs/config/effectChannels", [("i", 3)])

# ---------------------------------------------------------------------------
# THE TYPING GATE. Every one of these is a well-formed datagram carrying an
# argument the address cannot mean, and every one of them used to be ACCEPTED.
#
# They are sent AFTER the accepted count write and after every value asserted
# above is already in place, so each assertion that follows is a statement
# about the REFUSAL - "the stored value is still the one the legitimate write
# put there" - and not about which of two accepted writes happened to be last.
# ---------------------------------------------------------------------------

# Sent FIRST, so the assertions below distinguish "the text was refused" from
# "nothing was ever written here". A QLab custom message types its unquoted
# arguments as STRINGS, so "3" arriving as text at an integer parameter must
# still be the number 3 - and must be stored as `3`, not `3.0`, because a
# property reloads as a string var and every isInt()-guarded read of "3.0"
# silently answers a default.
TYPING_ACCEPTED = [
    ("/wfs/config/effects/workerThreads", [("s", "3")]),
]

TYPING_REFUSALS = [
    # THE DESTRUCTIVE ONE. static_cast<int> of a var holding "seven" is 0, and
    # 0 effect channels is EVERY EFFECT CHANNEL DELETED - silently, with the
    # write reported as a success, and with no undo (setNumEffectChannels is a
    # structural edit). This is the reason the typing gate exists.
    ("/wfs/config/effectChannels",          [("s", "seven")]),
    # The count is a whole number with a ceiling. setNumEffectChannels jlimits
    # 0..32 in silence, which turns "set 40" into "delete 8" and "set 2.5" into
    # "delete 1" - so the parser refuses both rather than letting the clamp
    # answer a question nobody asked.
    ("/wfs/config/effectChannels",          [("f", 2.5)]),
    ("/wfs/config/effectChannels",          [("i", 40)]),
    # Free text at the numeric effects globals. These are engine prepare()
    # inputs, and they used to persist the word verbatim.
    ("/wfs/config/effects/workerThreads",   [("s", "many")]),
    ("/wfs/config/effects/maxDelaySeconds", [("s", "ages")]),
    # An argument the shape cannot spend. minimalLatency is not fade-capable,
    # so a trailing number cannot be a transition time: either it is one
    # argument too many or the sender used the wrong address, and under the
    # second reading the "value" already read is an INDEX. Storing the 0 would
    # report a value nobody sent.
    ("/wfs/effect/minimalLatency",          [("i", 1), ("i", 0), ("i", 9)]),
    # ...and one argument too many at a scalar that IS fade-capable.
    ("/wfs/effect/attenuation",             [("i", 1), ("f", -5.0), ("f", 1.0), ("f", 2.0)]),
    # A channel that does not exist. The parser bounds the id against
    # maxEffectChannels (32), not the LIVE count, so these parse as valid and
    # then evaporated: setEffectParameter returns void on an invalid tree and
    # the instanced/band/tap arms were `if (node.isValid())` with no else.
    ("/wfs/effect/attenuation",             [("i", 9), ("f", -9.5)]),
    ("/wfs/effect/EQgain",                  [("i", 9), ("i", 2), ("i", 3), ("f", -9.5)]),
    # A misspelling and a missing value, which returned with an EMPTY reason
    # and so were not even reported as refused.
    ("/wfs/effect/attenuatino",             [("i", 1), ("f", -9.5)]),
    ("/wfs/effect/attenuation",             [("i", 1)]),
]

# The refusal reasons that must reach the SESSION log. OSCLogger starts
# disabled and the only thing that enables it is a human ticking the switch in
# the Network Log window, so a reason that goes only to logRejected is
# discarded on every unattended machine - which is every machine running a
# show. "Refused with a reason in the log, never silently dropped" is the
# contract of this parser, and this is the half of it that can be measured.
EXPECTED_LOG_REASONS = [
    "OSC refused /wfs/config/effectChannels",
    "OSC refused /wfs/config/effects/workerThreads",
    "OSC refused /wfs/effect/attenuatino",
    "OSC refused /wfs/effect/minimalLatency",
    "does not exist",
]

# The fixture has 16 outputs.
EXPECTED_MUTES = {
    "input7.mutes": ["0,1,1,0,1,0,0,0,0,0,0,0,0,0,0,0"],
    "input8.mutes": ["0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0"],
}

# Two single-output mutes on one input sent back to back, the way a QLab group
# fires its cues at once. Both must land: the ingest queue merges messages of
# one address and channel newest-wins, so /wfs/input/mutes bypasses it.
MUTE_BURST = [
    ("/wfs/input/mutes", [("i", 8), ("i", 2), ("i", 1)]),
    ("/wfs/input/mutes", [("i", 8), ("i", 7), ("i", 1)]),
]

# Values that OSCQuery cannot report: the pre-EQ descriptor publishes one
# node for all four bands and carries no VALUE, so these are read through the
# MCP read tool instead. (label, variable, channel_id, band, expected)
EQ_READS = [
    ("reverb1.preEQgain.band2", "reverbPreEQgain", 1, 2, -6.5),
    ("reverb1.preEQfreq.band3", "reverbPreEQfreq", 1, 3, 250.0),
]

# OSCQuery read-back paths for every touched value.
READS = [
    ("input1.positionX",  "/wfs/input/1/positionX"),   # -3.25 (500 rejected)
    ("input2.attenuation", "/wfs/input/2/attenuation"),
    ("input3.name",       "/wfs/input/3/name"),
    ("input4.cluster",    "/wfs/input/4/cluster"),
    ("input4.positionX",  "/wfs/input/4/positionX"),   # fixture pos + 0.5
    ("input4.positionY",  "/wfs/input/4/positionY"),   # fixture pos + 0.25
    ("output1.positionX", "/wfs/output/1/positionX"),
    ("reverb1.positionX", "/wfs/reverb/1/positionX"),
    ("stage.width",       "/wfs/config/stage/width"),
    ("input5.positionX",  "/wfs/input/5/positionX"),   # 1.75 + 0.25 = 2.0
    ("input6.positionX",  "/wfs/input/6/positionX"),   # polar R=2 result
    ("input6.positionY",  "/wfs/input/6/positionY"),
    ("input7.mutes",      "/wfs/input/7/mutes"),       # list + output 3, scalar refused
    ("input8.mutes",      "/wfs/input/8/mutes"),       # burst: outputs 2 and 7
]


def _round(v):
    if isinstance(v, float):
        return round(v, 6)
    if isinstance(v, list):
        return [_round(x) for x in v]
    return v


# ---------------------------------------------------------------------------
# Fixture preparation and effects.xml readers
# ---------------------------------------------------------------------------

def set_fixture_effect_channels(project: Path, count: int) -> None:
    """Rewrite <IO effectChannels="N"> in the TEMP copy's system.xml.

    applyConfigSection builds the whole effects family from this attribute, so
    the app comes up with `count` live channels, no UI and no OSC involved.
    Deliberately not the /wfs/config/effectChannels route: the gate must not
    depend on the mechanism it tests."""
    path = project / "system.xml"
    text = path.read_text(encoding="utf-8")
    new_text, n = re.subn(r'effectChannels="\d+"',
                          f'effectChannels="{count}"', text, count=1)
    if n != 1:
        raise SystemExit(f"[osc-replay] could not set effectChannels in {path}")
    path.write_text(new_text, encoding="utf-8")


def effect_node(root: ET.Element, effect_id: int) -> ET.Element:
    """The <Effect id="N"> element, found BY TYPE the way getEffectState
    counts - not by child index."""
    effects = root.find("Effects")
    if effects is None:
        raise AssertionError("effects.xml has no <Effects>")
    nth = 0
    for child in effects:
        if child.tag != "Effect":
            continue
        nth += 1
        if nth == effect_id:
            return child
    raise AssertionError(f"effects.xml has no effect {effect_id}")


def sub_node(effect: ET.Element, path: str) -> ET.Element:
    """`FxEq2/Band@3` -> the <Band id="3"> under <FxEq2>."""
    node = effect
    for step in path.split("/"):
        if "@" in step:
            tag, ident = step.split("@")
            found = None
            for child in node:
                if child.tag == tag and child.get("id") == ident:
                    found = child
                    break
            if found is None:
                raise AssertionError(f"no <{tag} id=\"{ident}\"> under <{node.tag}>")
            node = found
        else:
            found = node.find(step)
            if found is None:
                raise AssertionError(f"no <{step}> under <{node.tag}>")
            node = found
    return node


def attr_float(effect: ET.Element, path: str, name: str) -> float:
    raw = sub_node(effect, path).get(name)
    if raw is None:
        raise AssertionError(f"{path} carries no {name}")
    return float(raw)


def attr_text(effect: ET.Element, path: str, name: str) -> str:
    raw = sub_node(effect, path).get(name)
    if raw is None:
        raise AssertionError(f"{path} carries no {name}")
    return raw


def row_tokens(effect: ET.Element, name: str) -> list[str]:
    return attr_text(effect, "Sends", name).split(",")


def newest_session_log(after: float) -> Path | None:
    log_dir = Path(os.environ.get("APPDATA", "")) / "WFS-DIY" / "logs"
    if not log_dir.is_dir():
        return None
    candidates = [p for p in log_dir.glob("WFS-DIY_*.log")
                  if p.stat().st_mtime >= after]
    if not candidates:
        return None
    return max(candidates, key=lambda p: p.stat().st_mtime)


# ---------------------------------------------------------------------------
# The effects assertions. Every one names the node, not only the value.
# ---------------------------------------------------------------------------

def check_effects(project: Path, failures: list[str]) -> dict:
    """Assert on the saved effects.xml; returns the read-backs worth pinning
    in the golden."""
    tree = ET.parse(project / "effects.xml")
    root = tree.getroot()
    readbacks: dict[str, object] = {}

    def want(label, got, expected):
        readbacks[label] = _round(got)
        if got != expected:
            failures.append(f"{label}: expected {expected!r}, got {got!r}")

    fx1 = effect_node(root, 1)
    fx2 = effect_node(root, 2)

    # -- scalars, both wire forms
    want("fx1.attenuation", attr_float(fx1, "Channel", "effectAttenuation"), -12.5)
    want("fx2.positionX", attr_float(fx2, "Position", "effectPositionX"), -3.25)

    # -- instanced: the point is that they are DIFFERENT and on different nodes
    want("fx1.dyn1.threshold",
         attr_float(fx1, "FxDyn1", "effectDynCompThreshold"), -18.5)
    want("fx1.dyn2.threshold",
         attr_float(fx1, "FxDyn2", "effectDynCompThreshold"), -30.25)

    # -- band: instance AND band both have to be honoured
    want("fx1.eq1.band1.gain", attr_float(fx1, "FxEq1/Band@1", "effectEQgain"), 4.5)
    want("fx1.eq2.band3.gain", attr_float(fx1, "FxEq2/Band@3", "effectEQgain"), -6.5)
    # ...and nothing landed on the bands NOT named. An EQ write that ignores its
    # indices lands on instance 1 band 1, which the two lines above would still
    # pass if the values happened to match.
    want("fx1.eq2.band1.gain", attr_float(fx1, "FxEq2/Band@1", "effectEQgain"), 0.0)
    want("fx1.eq1.band3.gain", attr_float(fx1, "FxEq1/Band@3", "effectEQgain"), 0.0)

    # -- delay tap 5, and tap 1 untouched
    want("fx1.tap5.time", attr_float(fx1, "FxDelay/Tap@5", "effectDelayTapTime"), 250.0)
    tap1 = attr_float(fx1, "FxDelay/Tap@1", "effectDelayTapTime")
    if tap1 == 250.0:
        failures.append("fx1.tap1.time: the tap-5 write landed on tap 1")
    readbacks["fx1.tap1.time"] = _round(tap1)

    # -- send cells, both keyings, and the columns around them left alone
    in_levels = row_tokens(fx1, "effectSendLevels")
    in_ons = row_tokens(fx1, "effectSendOns")
    want("fx1.sendLevels.width", len(in_levels), 64)
    want("fx1.sendLevel.in3", float(in_levels[2]), -24.5)
    want("fx1.sendOn.in3", int(float(in_ons[2])), 1)
    want("fx1.sendOn.in2", int(float(in_ons[1])), 0)

    fx_levels = row_tokens(fx2, "effectFxSendLevels")
    fx_ons = row_tokens(fx2, "effectFxSendOns")
    want("fx2.fxSendLevels.width", len(fx_levels), 32)
    want("fx2.fxSendLevel.fx1", float(fx_levels[0]), -12.25)
    want("fx2.fxSendOn.fx1", int(float(fx_ons[0])), 1)
    # The diagonal stays off although it was explicitly addressed above: effect
    # 2 may not feed itself. Mutation-tested, and the result is worth recording -
    # the diagonal is defended TWICE and removing setEffectFxSendOnFromEffect's
    # early return alone does NOT move this cell, because the write interceptor
    # re-forces it through canonicalEffectSendRow (which is where a whole-ROW
    # write would otherwise smuggle a self-send in). It takes removing both to
    # make this assertion fail, which it then does: expected 0, got 1.
    want("fx2.fxSendOn.self", int(float(fx_ons[1])), 0)

    # -- the packed rows
    sends2 = row_tokens(fx2, "effectSendOns")
    want("fx2.sendOns.width", len(sends2), 64)
    want("fx2.sendOns.head", ",".join(sends2[:4]), "0,1,0,1")
    want("fx1.chainOrder", attr_text(fx1, "Chain", "effectChainOrder"),
         "eq1,dist,eq2,dyn1,dyn2,mod,phaser,trem,reverb,delay,crush")

    # -- THE BURST: all 64 cells of effect 2's input-keyed level row
    burst = row_tokens(fx2, "effectSendLevels")
    want("fx2.sendLevels.width", len(burst), 64)
    landed = sum(1 for k in range(1, 65) if float(burst[k - 1]) == float(-k))
    want("fx2.sendLevels.burstLanded", landed, 64)

    # -- the globals, read out of system.xml where they actually live
    system_root = ET.parse(project / "system.xml").getroot()
    config = system_root.find("Config")
    eff_global = config.find("EffectsGlobal")
    master = config.find("Master")
    want("global.linkMode", int(eff_global.get("effectsGlobalLinkMode")), 2)
    want("global.loopGuardCeil", float(eff_global.get("effectsGlobalLoopGuardCeiling")), 3.5)
    want("global.mapVisible", int(master.get("effectsMapVisible")), 0)

    # -- the count write was accepted while stopped
    count = sum(1 for c in root.find("Effects") if c.tag == "Effect")
    want("effects.count", count, 3)
    want("effects.countAttr", int(root.find("Effects").get("count")), 3)
    # and it did not disturb what was already there
    want("fx1.attenuation.afterCount",
         attr_float(effect_node(root, 1), "Channel", "effectAttenuation"), -12.5)

    # -- THE TYPING GATE, all of it read back after TYPING_REFUSALS was sent.
    #
    # The count assertions immediately above are the first half of it and the
    # strongest: "seven" at the channel count used to read as 0 and delete
    # every <Effect> in the file, so a regression there does not fail one line,
    # it fails most of this function. 2.5 and 40 would have landed as 2 and 32
    # through setNumEffectChannels' silent clamp.
    #
    # `fx1.attenuation.afterCount` is the second half: the four-argument write
    # in TYPING_REFUSALS would have stored -5.0 over it.

    # The QLab string "3" became the NUMBER 3 (not the text "many" that
    # followed it, and not "3.0" - an int-typed parameter is stored as an int).
    want("global.workerThreads.afterText",
         eff_global.get("effectsGlobalWorkerThreads"), "3")
    # Never written by this driver, so this one is the default surviving free
    # text rather than a legitimate write surviving it.
    want("global.maxDelaySeconds.afterText",
         eff_global.get("effectsGlobalMaxDelaySeconds"), "5")

    # The trailing 9 was not a fade and the 0 was not a value: minimalLatency
    # keeps its default. Sent as `1 0 9` precisely so the refused value (0)
    # differs from the default (1) - a refusal that stored the value would
    # read 0 here.
    want("fx1.minimalLatency.afterArity",
         int(sub_node(fx1, "Channel").get("effectMinimalLatency")), 1)

    # Effect 9 does not exist, so nothing it was sent may appear anywhere: no
    # channel was created (the count assertions above) and the EQ band named in
    # the same breath is still where the legitimate write left it.
    want("fx1.eq2.band3.gain.afterGhost",
         attr_float(fx1, "FxEq2/Band@3", "effectEQgain"), -6.5)

    return readbacks


def run_stopped_pass(exe: Path, keep_temp: bool, failures: list[str]) -> dict:
    """The main pass: the whole write script against a stopped engine."""
    work_root = Path(os.environ.get("TEMP", ".")) / "wfs-control-replay" / "osc_replay"
    project = common.copy_fixture_to_temp(work_root)
    set_fixture_effect_channels(project, EFFECT_CHANNELS)

    started_at = time.time() - 1.0
    common.kill_stale_instances()
    # ai_enabled: the pre-EQ band values are read back over MCP (see
    # EQ_READS) and the effects family is read back from a project saved with
    # the tier-2 `session_save` tool. The master AI toggle refuses every tool
    # call while it is off. This driver makes no MCP parameter writes.
    app = common.App(exe, common.fixture_wfs(project), ai_enabled=True)
    readbacks: dict[str, object] = {}
    try:
        app.wait_for_mcp()
        # OSC listening + OSCQuery both come up when network.xml is applied.
        app.wait_for_oscquery()

        # The app's OSC ingest queue COALESCES rapid same-(address, channel)
        # updates (OSCIngestQueue) — two quick writes to one slot keep only
        # the newest. The scripted sequence relies on write ordering (an
        # absolute write followed by an inc-delta on the same slot; a valid
        # write followed by an out-of-range write on the same slot), so
        # every send gets a generous settle delay to guarantee it drains on
        # the message thread before the next packet arrives.
        sender = common.OSCSender(delay=0.35)
        for label, address, osc_args in WRITES:
            sender.send(address, osc_args)
        for label, address, osc_args in EFFECT_WRITES:
            sender.send(address, osc_args)
        sender.close()

        burst = common.OSCSender(delay=0.0)
        for address, osc_args in MUTE_BURST:
            burst.send(address, osc_args)
        # One datagram train, no spacing: this is the case the bypass list and
        # the sub-indexed coalesce key exist for.
        for address, osc_args in CELL_BURST:
            burst.send(address, osc_args)
        burst.close()

        time.sleep(1.5)

        # LAST, so nothing above is written to a channel that did not exist
        # yet: the structural write. Accepted here because processing is
        # stopped; RUNNING_REFUSAL covers the other half.
        tail = common.OSCSender(delay=0.5)
        tail.send(*COUNT_WRITE)
        tail.close()

        # THE TYPING GATE, last of all: every value asserted below is already
        # in place, so what these prove is that a refusal leaves it there.
        typing = common.OSCSender(delay=0.35)
        for address, osc_args in TYPING_ACCEPTED:
            typing.send(address, osc_args)
        for address, osc_args in TYPING_REFUSALS:
            typing.send(address, osc_args)
        typing.close()

        # Final drain before reading back.
        time.sleep(1.5)

        for label, path in READS:
            try:
                readbacks[label] = _round(common.oscquery_get(path))
            except Exception as exc:  # noqa: BLE001 — record, don't die
                readbacks[label] = f"<read failed: {exc}>"

        for label, variable, channel_id, band, _expected in EQ_READS:
            try:
                payload = common.tool_payload(app.tool("wfs_get_parameter", {
                    "variable": variable,
                    "channel_id": channel_id,
                    "band": band,
                }))
                readbacks[label] = _round(payload.get("value"))
            except Exception as exc:  # noqa: BLE001 — record, don't die
                readbacks[label] = f"<read failed: {exc}>"

        # The effects family has no OSCQuery namespace yet, so it is read out
        # of the saved project instead.
        first, final = app.tool_confirmed("session_save", {})
        payload = common.tool_payload(final)
        if not (isinstance(payload, dict) and payload.get("saved") is True):
            failures.append(f"session_save failed, effects unreadable: {payload}")
    finally:
        app.close()

    try:
        readbacks.update(check_effects(project, failures))
    except AssertionError as exc:
        failures.append(f"effects.xml: {exc}")
    except Exception as exc:  # noqa: BLE001
        failures.append(f"effects.xml could not be read: {exc}")

    # THE DROP COUNTER. The ingest FIFO is 256 deep and shared; a 64-cell burst
    # uses a quarter of it. The overflow report reaches the session log (as well
    # as the in-app Network table), so its absence is the assertion.
    log = newest_session_log(started_at)
    if log is None:
        failures.append("no session log found: the ingest drop counter was not checked")
    else:
        log_text = log.read_text(encoding="utf-8", errors="replace")
        dropped = [ln.strip() for ln in log_text.splitlines()
                   if "ingest queue full" in ln]
        if dropped:
            failures.append(f"the ingest queue dropped messages: {dropped[:3]}")

        # THE OTHER HALF OF THE TYPING GATE: the reasons have to reach a place
        # somebody reads. See EXPECTED_LOG_REASONS - logRejected alone lands in
        # a window that is switched off until a human switches it on, so every
        # assertion above about a value NOT moving would be satisfied just as
        # well by a message that vanished without a word.
        for needle in EXPECTED_LOG_REASONS:
            if needle not in log_text:
                failures.append(
                    f"no session-log line for a refusal: {needle!r} - a refused "
                    f"message was dropped without telling anybody")

    if not keep_temp:
        shutil.rmtree(work_root, ignore_errors=True)
    return readbacks


def run_running_refusal(exe: Path, keep_temp: bool, failures: list[str]) -> None:
    """The other half of the stopped-only gate.

    A second, short app run with WFS_TEST_AUTOSTART_PROCESSING=1, which flips
    <IO runDSP> through the same seam the Start button uses. A project can
    never LOAD with DSP flagged running (WFSFileManager::stripTransientToggles
    removes the toggle on both save and load, by design), so the flag has to
    come from the app itself.

    While it is running, /wfs/config/effectChannels must be refused and the
    count must not move - and an ordinary effect write must still land, so a
    refusal cannot be mistaken for "OSC stopped working"."""
    work_root = Path(os.environ.get("TEMP", ".")) / "wfs-control-replay" / "osc_replay_running"
    project = common.copy_fixture_to_temp(work_root)
    set_fixture_effect_channels(project, EFFECT_CHANNELS)

    started_at = time.time() - 1.0
    common.kill_stale_instances()
    env_key = "WFS_TEST_AUTOSTART_PROCESSING"
    had = env_key in os.environ
    previous = os.environ.get(env_key)
    os.environ[env_key] = "1"
    try:
        app = common.App(exe, common.fixture_wfs(project), ai_enabled=True)
        try:
            app.wait_for_mcp()
            app.wait_for_oscquery()
            # The autostart hook fires 1.5 s after the project opens.
            time.sleep(4.0)

            sender = common.OSCSender(delay=0.5)
            sender.send("/wfs/config/effectChannels", [("i", 7)])
            sender.send("/wfs/effect/attenuation", [("i", 1), ("f", -21.5)])
            sender.close()
            time.sleep(1.5)

            _first, final = app.tool_confirmed("session_save", {})
            payload = common.tool_payload(final)
            if not (isinstance(payload, dict) and payload.get("saved") is True):
                failures.append(f"running pass: session_save failed: {payload}")
        finally:
            app.close()
    finally:
        if had:
            os.environ[env_key] = previous  # type: ignore[arg-type]
        else:
            os.environ.pop(env_key, None)

    # THE PASS MUST NOT SUCCEED BECAUSE NOTHING STARTED. A refusal gate whose
    # precondition never held is a gate that cannot fail, which is how three
    # assertions on this branch shipped green and meaningless. Processing
    # starting is what makes the refusal meaningful, so it is asserted first.
    log = newest_session_log(started_at)
    if log is None:
        failures.append("running pass: no session log, so it is unknown whether "
                        "processing ever started")
    else:
        text = log.read_text(encoding="utf-8", errors="replace")
        if "Processing enabled" not in text:
            failures.append("running pass: processing never started, so the "
                            "stopped-only refusal was not exercised at all")

    try:
        system_text = (project / "system.xml").read_text(encoding="utf-8")
        match = re.search(r'effectChannels="(\d+)"', system_text)
        if match is None:
            failures.append("running pass: system.xml carries no effectChannels")
        elif match.group(1) != str(EFFECT_CHANNELS):
            failures.append(
                "running pass: the count changed while processing was running "
                f"(expected {EFFECT_CHANNELS}, got {match.group(1)}) — a remote "
                "client can stop a live show")

        root = ET.parse(project / "effects.xml").getroot()
        live = sum(1 for c in root.find("Effects") if c.tag == "Effect")
        if live != EFFECT_CHANNELS:
            failures.append(f"running pass: {live} effect channels were built, "
                            f"expected {EFFECT_CHANNELS}")
        atten = attr_float(effect_node(root, 1), "Channel", "effectAttenuation")
        if atten != -21.5:
            failures.append("running pass: an ordinary effect write did not land "
                            f"while processing was running (got {atten}) — the "
                            "refusal must be the COUNT, not the family")
    except Exception as exc:  # noqa: BLE001
        failures.append(f"running pass: could not read back: {exc}")

    if not keep_temp:
        shutil.rmtree(work_root, ignore_errors=True)


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--exe", default=None)
    p.add_argument("--update", action="store_true",
                   help="Rewrite the golden with this run's read-backs")
    p.add_argument("--keep-temp", action="store_true")
    p.add_argument("--skip-running-pass", action="store_true",
                   help="Skip the second app run that starts processing")
    args = p.parse_args()

    exe = common.find_exe(args.exe)

    failures: list[str] = []
    readbacks = run_stopped_pass(exe, args.keep_temp, failures)
    if not args.skip_running_pass:
        run_running_refusal(exe, args.keep_temp, failures)

    actual_text = json.dumps(readbacks, indent=2, sort_keys=True) + "\n"

    # Hard invariants independent of the golden: the rejected write must
    # not have landed, and the inc-delta must have applied exactly once.
    ok = True
    if readbacks.get("input1.positionX") != [-3.25]:
        print("[osc-replay] HARD FAIL: out-of-range write was not rejected "
              f"keep-current: {readbacks.get('input1.positionX')}",
              file=sys.stderr)
        ok = False
    if readbacks.get("input5.positionX") != [2.0]:
        print("[osc-replay] HARD FAIL: inc delta result wrong: "
              f"{readbacks.get('input5.positionX')}", file=sys.stderr)
        ok = False
    for label, _variable, _channel_id, band, expected in EQ_READS:
        got = readbacks.get(label)
        if got != expected:
            print(f"[osc-replay] HARD FAIL: reverb pre-EQ band {band} write did "
                  f"not land ({label}): expected {expected}, got {got}",
                  file=sys.stderr)
            ok = False
    for label, expected in EXPECTED_MUTES.items():
        if readbacks.get(label) != expected:
            print(f"[osc-replay] HARD FAIL: {label} is {readbacks.get(label)}, "
                  f"expected {expected}", file=sys.stderr)
            ok = False
    for failure in failures:
        print(f"[osc-replay] HARD FAIL: {failure}", file=sys.stderr)
        ok = False

    if not common.compare_or_update(GOLDEN, actual_text, args.update,
                                    "osc-replay"):
        ok = False
    return common.EXIT_PASS if ok else common.EXIT_MISMATCH


if __name__ == "__main__":
    raise SystemExit(main())
