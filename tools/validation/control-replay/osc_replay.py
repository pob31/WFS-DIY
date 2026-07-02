"""OSC replay driver.

Launches WFS-DIY on a temp copy of the golden fixture, sends a scripted,
deterministic OSC write sequence over UDP 8000 covering the address
families (per-channel float/string/int writes, config-global, cluster
move, /remoteInput absolute + inc delta, one polar write, one out-of-range
write that must be rejected keep-current), then reads every touched value
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
    ("output1.positionX",  "/wfs/output/positionX",  [("i", 1), ("f", -4.5)]),
    ("reverb1.positionX",  "/wfs/reverb/positionX",  [("i", 1), ("f", 2.25)]),
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
    app = common.App(exe, common.fixture_wfs(project), ai_enabled=False)
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

        # Final drain before reading back.
        time.sleep(1.5)

        readbacks = {}
        for label, path in READS:
            try:
                readbacks[label] = _round(common.oscquery_get(path))
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

    if not common.compare_or_update(GOLDEN, actual_text, args.update,
                                    "osc-replay"):
        ok = False
    return common.EXIT_PASS if ok else common.EXIT_MISMATCH


if __name__ == "__main__":
    raise SystemExit(main())
