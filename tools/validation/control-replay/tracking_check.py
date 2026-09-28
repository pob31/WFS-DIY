"""Tracking ingress check (audit 2026-09-28, N6).

Launches WFS-DIY on a temp copy of the golden fixture with OSC tracking
switched on (protocol 1, UDP 7000, the fixture's `/wfs/tracking <ID> <x> <y>
<z>` pattern) and two inputs tracking:

  input 1, smoothing 100 %: a NaN sample, then a good one. One NaN used to
          poison the input's position filter for good (a NaN distance never
          trips the jump test that would reset it), so the good sample came
          out NaN. It must land as sent: the filter's first accepted sample
          passes through unchanged.
  input 2, smoothing 0 %: a finite but absurd 1e30 m sample, which used to be
          written into the offset as it came. Tracking now writes through the
          store, whose range is -50..50 m.

Offsets are read back over OSCQuery. PSN, RTTrP and MQTT share the gate in
spatcore's TrackingIngestQueue, which spatcore-tests cover.

Usage:
  python tracking_check.py [--exe path] [--keep-temp]

Exit codes: 0 pass, 1 fail, 2 usage, 3 app failed to start.
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import sys
import time
from pathlib import Path

import common

TRACKING_PORT = 7000     # trackingPort in the fixture
NAN = float("nan")


def patch_fixture(project: Path) -> None:
    network = project / "network.xml"
    text = network.read_text(encoding="utf-8")
    text = text.replace('trackingEnabled="0"', 'trackingEnabled="1"', 1)
    text = re.sub(r'trackingProtocol="\d+"', 'trackingProtocol="1"', text, count=1)
    network.write_text(text, encoding="utf-8")

    inputs = project / "inputs.xml"
    text = inputs.read_text(encoding="utf-8")
    # The first two <Input> elements: tracking on; the second unsmoothed.
    parts = re.split(r'(<Input\b)', text)
    seen = 0
    for i in range(1, len(parts), 2):
        seen += 1
        if seen > 2:
            break
        body = parts[i + 1]
        body = body.replace('inputTrackingActive="0"', 'inputTrackingActive="1"', 1)
        if seen == 2:
            body = body.replace('inputTrackingSmooth="100"', 'inputTrackingSmooth="0"', 1)
        parts[i + 1] = body
    inputs.write_text("".join(parts), encoding="utf-8")


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--exe", default=None)
    p.add_argument("--keep-temp", action="store_true")
    args = p.parse_args()

    exe = common.find_exe(args.exe)
    work_root = Path(os.environ.get("TEMP", ".")) / "wfs-control-replay" / "tracking_check"
    project = common.copy_fixture_to_temp(work_root)
    patch_fixture(project)

    failures: list[str] = []
    readbacks: dict[str, object] = {}

    common.kill_stale_instances()
    app = common.App(exe, common.fixture_wfs(project), ai_enabled=False)
    try:
        app.wait_for_oscquery()
        time.sleep(1.0)   # the tracking receiver starts with network.xml

        tracker = common.OSCSender(port=TRACKING_PORT, delay=0.3)
        tracker.send("/wfs/tracking", [("i", 1), ("f", NAN), ("f", 0.0), ("f", 0.0)])
        tracker.send("/wfs/tracking", [("i", 1), ("f", 1.25), ("f", 0.5), ("f", 0.0)])
        tracker.send("/wfs/tracking", [("i", 2), ("f", 1e30), ("f", 0.25), ("f", 0.0)])
        tracker.close()
        time.sleep(1.0)

        for label, path in (("input1.offsetX", "/wfs/input/1/offsetX"),
                            ("input1.offsetY", "/wfs/input/1/offsetY"),
                            ("input2.offsetX", "/wfs/input/2/offsetX"),
                            ("input2.offsetY", "/wfs/input/2/offsetY")):
            try:
                readbacks[label] = common.oscquery_get(path)
            except Exception as exc:  # noqa: BLE001
                readbacks[label] = f"<read failed: {exc}>"
        if not app.alive():
            failures.append("the app died during the tracking run")
    finally:
        app.close()

    expected = {
        "input1.offsetX": [1.25],   # the NaN before it was dropped, not filtered
        "input1.offsetY": [0.5],
        "input2.offsetX": [50.0],   # 1e30 m, clamped by the store
        "input2.offsetY": [0.25],
    }
    for label, want in expected.items():
        got = readbacks.get(label)
        ok = isinstance(got, list) and len(got) == 1 and isinstance(got[0], (int, float)) \
            and abs(got[0] - want[0]) < 1e-4
        print(f"[tracking] {'PASS' if ok else 'FAIL'}  {label} = {got} (expected {want})")
        if not ok:
            failures.append(label)

    if not args.keep_temp:
        shutil.rmtree(work_root, ignore_errors=True)

    for failure in failures:
        print(f"[tracking] HARD FAIL: {failure}", file=sys.stderr)
    if not failures:
        print("[tracking] ALL PASS")
    return common.EXIT_PASS if not failures else common.EXIT_MISMATCH


if __name__ == "__main__":
    raise SystemExit(main())
