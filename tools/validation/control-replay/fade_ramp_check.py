"""OSC fade-time (transition seconds) regression check.

Drives a live app over UDP OSC and samples parameter values through the
OSCQuery HTTP endpoint while ramps run. Asserts the contract of the optional
trailing fade argument on /wfs/input paths:

  1. numeric fade -> value interpolates and lands on target on time
  2. fade sent as a STRING numeral (QLab types custom-message args as
     strings) -> same ramp
  3. int value + int fade -> ramp
  4. no fade arg -> instant
  5. non-fade-capable param with trailing number -> instant (arg ignored)
  6. newly fade-capable params ramp: attenuation, commonAtten (integer
     steps), arrayAtten (also proves the new arrayAtten OSC routing)
  7. huge fade clamps to 600 s -> slow crawl, not a jump
  8. instant write during a ramp cancels the ramp (newer write wins)

Assertion-based (no golden). Exit codes per common.py contract.

Usage: python fade_ramp_check.py [--exe PATH] [--keep-temp]
"""

from __future__ import annotations

import argparse
import os
import shutil
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common  # noqa: E402


FAILURES: list[str] = []


def check(cond: bool, label: str, detail: str = "") -> None:
    if cond:
        print(f"[fade-ramp] PASS  {label}")
    else:
        FAILURES.append(label)
        print(f"[fade-ramp] FAIL  {label}  {detail}", file=sys.stderr)


def read1(path: str) -> float:
    v = common.oscquery_get(path)
    if isinstance(v, list):
        v = v[0]
    return float(v)


def sample(path: str, duration: float, period: float = 0.1
           ) -> list[tuple[float, float]]:
    """Poll an OSCQuery value for `duration` seconds."""
    out: list[tuple[float, float]] = []
    t0 = time.monotonic()
    while (t := time.monotonic() - t0) < duration:
        out.append((t, read1(path)))
        time.sleep(period)
    return out


def reach_time(trace: list[tuple[float, float]], target: float,
               tol: float) -> float | None:
    for t, v in trace:
        if abs(v - target) <= tol:
            return t
    return None


def distinct_between(trace: list[tuple[float, float]], lo: float, hi: float
                     ) -> int:
    eps = (hi - lo) * 0.02
    vals = {round(v, 4) for _, v in trace if lo + eps < v < hi - eps}
    return len(vals)


def run(app: common.App) -> None:
    send = common.OSCSender(delay=0.0).send
    DA = "/wfs/input/distanceAttenuation"
    DA_Q = "/wfs/input/1/distanceAttenuation"
    MS = "/wfs/input/maxSpeed"
    MS_Q = "/wfs/input/1/maxSpeed"

    def reset(address: str, args: list) -> None:
        send(address, args)
        time.sleep(0.6)

    # -- 1. numeric fade ramps on time --------------------------------------
    reset(DA, [("i", 1), ("f", -0.7)])
    send(DA, [("i", 1), ("f", -3.0), ("f", 2.0)])
    trace = sample(DA_Q, 3.5)
    rt = reach_time(trace, -3.0, 0.02)
    check(rt is not None and 1.5 <= rt <= 3.2,
          "float fade 2.0 s reaches target on time",
          f"reach={rt} trace-end={trace[-1] if trace else None}")
    check(distinct_between(trace, -3.0, -0.7) >= 3,
          "float fade interpolates (intermediate values seen)",
          f"distinct={distinct_between(trace, -3.0, -0.7)}")

    # -- 2. fade as string numeral (QLab) -----------------------------------
    reset(DA, [("i", 1), ("f", -0.7)])
    send(DA, [("i", 1), ("s", "-3.000000"), ("s", "2.0")])
    trace = sample(DA_Q, 3.5)
    rt = reach_time(trace, -3.0, 0.02)
    check(rt is not None and 1.5 <= rt <= 3.2,
          "string value + string fade ramps (QLab tokenising)",
          f"reach={rt} end={trace[-1] if trace else None}")
    check(distinct_between(trace, -3.0, -0.7) >= 3,
          "string-arg fade interpolates",
          f"distinct={distinct_between(trace, -3.0, -0.7)}")

    # -- 3. int value + int fade --------------------------------------------
    reset(MS, [("i", 1), ("f", 1.0)])
    send(MS, [("i", 1), ("i", 5), ("i", 2)])
    trace = sample(MS_Q, 3.5)
    rt = reach_time(trace, 5.0, 0.02)
    check(rt is not None and 1.5 <= rt <= 3.2,
          "int value + int fade ramps", f"reach={rt}")

    # -- 4. no fade arg -> instant ------------------------------------------
    reset(MS, [("i", 1), ("f", 1.0)])
    send(MS, [("i", 1), ("f", 4.0)])
    time.sleep(0.5)
    check(abs(read1(MS_Q) - 4.0) < 1e-3, "no fade arg applies instantly")

    # -- 5. non-capable param ignores the fade arg gracefully ----------------
    send("/wfs/input/trackingSmooth", [("i", 1), ("i", 50), ("f", 3.0)])
    time.sleep(0.5)
    check(abs(read1("/wfs/input/1/trackingSmooth") - 50.0) < 1e-3,
          "non-fade-capable param applies instantly, arg ignored")

    # -- 6. newly fade-capable params ---------------------------------------
    reset("/wfs/input/attenuation", [("i", 1), ("f", 0.0)])
    send("/wfs/input/attenuation", [("i", 1), ("f", -12.0), ("f", 2.0)])
    trace = sample("/wfs/input/1/attenuation", 3.5)
    rt = reach_time(trace, -12.0, 0.05)
    check(rt is not None and 1.5 <= rt <= 3.2,
          "attenuation (level) fades", f"reach={rt}")

    reset("/wfs/input/commonAtten", [("i", 1), ("i", 100)])
    send("/wfs/input/commonAtten", [("i", 1), ("i", 40), ("f", 2.0)])
    trace = sample("/wfs/input/1/commonAtten", 3.5)
    rt = reach_time(trace, 40.0, 0.5)
    check(rt is not None and 1.5 <= rt <= 3.2,
          "commonAtten fades", f"reach={rt}")
    nonint = [v for _, v in trace if abs(v - round(v)) > 1e-4]
    check(not nonint, "commonAtten ramp stays integer-valued",
          f"non-integers={nonint[:4]}")

    reset("/wfs/input/arrayAtten3", [("i", 1), ("f", 0.0)])
    send("/wfs/input/arrayAtten3", [("i", 1), ("f", -20.0), ("f", 2.0)])
    trace = sample("/wfs/input/1/arrayAtten3", 3.5)
    rt = reach_time(trace, -20.0, 0.1)
    check(rt is not None and 1.5 <= rt <= 3.2,
          "arrayAtten3 routes over OSC and fades", f"reach={rt}")

    # -- 7. huge fade clamps to 600 s -> crawl, not jump ---------------------
    reset(MS, [("i", 1), ("f", 1.0)])
    send(MS, [("i", 1), ("f", 20.0), ("f", 10000.0)])
    time.sleep(2.0)
    v = read1(MS_Q)
    check(1.0 < v < 1.5,
          "fade 10000 s clamps to 600 s crawl (moved, but barely)",
          f"value after 2 s = {v}")

    # -- 8. instant write mid-ramp cancels the ramp --------------------------
    reset(MS, [("i", 1), ("f", 1.0)])
    send(MS, [("i", 1), ("f", 8.0), ("f", 4.0)])
    time.sleep(1.0)
    send(MS, [("i", 1), ("f", 3.0)])
    time.sleep(0.5)
    v1 = read1(MS_Q)
    time.sleep(1.0)
    v2 = read1(MS_Q)
    check(abs(v1 - 3.0) < 1e-3 and abs(v2 - 3.0) < 1e-3,
          "instant write mid-ramp wins and sticks",
          f"v after override: {v1}, 1 s later: {v2}")


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--exe", default=None)
    p.add_argument("--keep-temp", action="store_true")
    args = p.parse_args()

    exe = common.find_exe(args.exe)
    work_root = Path(os.environ.get("TEMP", ".")) / "wfs-control-replay" \
        / "fade_ramp"
    project = common.copy_fixture_to_temp(work_root)

    common.kill_stale_instances()
    app = common.App(exe, common.fixture_wfs(project), ai_enabled=False)
    try:
        app.wait_for_mcp()
        app.wait_for_oscquery()
        run(app)
    finally:
        app.close()

    if not args.keep_temp:
        shutil.rmtree(work_root, ignore_errors=True)

    if FAILURES:
        print(f"[fade-ramp] {len(FAILURES)} failure(s): {FAILURES}",
              file=sys.stderr)
        return common.EXIT_MISMATCH
    print("[fade-ramp] ALL PASS")
    return common.EXIT_PASS


if __name__ == "__main__":
    raise SystemExit(main())
