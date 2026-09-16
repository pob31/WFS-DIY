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
import shutil
import sys
import time
from pathlib import Path

import common

GOLDEN = common.GOLDENS_DIR / "osc_replay.json"

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

# Two single-output mutes on one input sent back to back, the way a QLab group
# fires its cues at once. Both must land: the ingest queue merges messages of
# one address and channel newest-wins, so /wfs/input/mutes bypasses it.
MUTE_BURST = [
    ("/wfs/input/mutes", [("i", 8), ("i", 2), ("i", 1)]),
    ("/wfs/input/mutes", [("i", 8), ("i", 7), ("i", 1)]),
]

# The fixture has 16 outputs.
EXPECTED_MUTES = {
    "input7.mutes": ["0,1,1,0,1,0,0,0,0,0,0,0,0,0,0,0"],
    "input8.mutes": ["0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0"],
}

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


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--exe", default=None)
    p.add_argument("--update", action="store_true",
                   help="Rewrite the golden with this run's read-backs")
    p.add_argument("--keep-temp", action="store_true")
    args = p.parse_args()

    exe = common.find_exe(args.exe)
    work_root = Path(os.environ.get("TEMP", ".")) / "wfs-control-replay" \
        / "osc_replay"
    project = common.copy_fixture_to_temp(work_root)

    common.kill_stale_instances()
    # ai_enabled: the pre-EQ band values are read back over MCP (see
    # EQ_READS), and the master AI toggle refuses every tool call while it is
    # off. This driver makes no MCP writes.
    app = common.App(exe, common.fixture_wfs(project), ai_enabled=True)
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
        sender.close()

        burst = common.OSCSender(delay=0.0)
        for address, osc_args in MUTE_BURST:
            burst.send(address, osc_args)
        burst.close()

        # Final drain before reading back.
        time.sleep(1.5)

        readbacks = {}
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
    finally:
        app.close()

    if not args.keep_temp:
        shutil.rmtree(work_root, ignore_errors=True)

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

    if not common.compare_or_update(GOLDEN, actual_text, args.update,
                                    "osc-replay"):
        ok = False
    return common.EXIT_PASS if ok else common.EXIT_MISMATCH


if __name__ == "__main__":
    raise SystemExit(main())
