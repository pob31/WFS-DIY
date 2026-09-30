"""Run WFS-DIY's env-gated self-tests headless.

Launches the app on a temp copy of the golden fixture with one or more
WFS_TEST_* variables set, waits for "SELF-TEST RESULT" in the session log the
launch created, closes the app and prints the SELF-TEST lines. The settings
file (%APPDATA%/WFS-DIY/WFS-DIY.settings, which the app rewrites on exit) is
backed up and restored around the run.

--device "type|name" opens that audio device instead of the saved one, for the
self-tests that need the device to call back (WFS_TEST_ENGINE_RECONFIG): a
saved ASIO device often cannot open in a session with no interactive desktop,
and then startAudioEngine logs "no audio device available". Example:
  --device "Windows Audio|Speakers (Cirrus Logic XU (with APO Extensions))"
(the endpoint names are under HKLM\\...\\MMDevices\\Audio\\Render).

A self-test that leaves processing running makes the app ask before quitting,
which the close cannot answer: end such tests with processing stopped.

Usage:
  python run_selftest.py --env WFS_TEST_VALUE_GATES [--exe path]
  python run_selftest.py --env WFS_TEST_AUTOSTART_PROCESSING,WFS_TEST_ENGINE_RECONFIG \\
                         --device "Windows Audio|<endpoint name>"

Exit codes: 0 ALL PASS, 1 FAIL / SKIPPED / no result, 3 app failed to start.
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import sys
import time
from pathlib import Path
from xml.sax.saxutils import quoteattr

import common


def inject_device(settings: Path, device: str) -> None:
    dev_type, dev_name = device.split("|", 1)
    text = settings.read_text(encoding="utf-8")
    for key, val in (("audioDeviceType", dev_type), ("audioDeviceName", dev_name)):
        text = re.sub(r'<VALUE name="' + key + r'" val="[^"]*"/>',
                      lambda m, k=key, v=val: '<VALUE name="' + k + '" val=' + quoteattr(v) + "/>",
                      text)
    state = ("<DEVICESETUP deviceType=" + quoteattr(dev_type)
             + " audioOutputDeviceName=" + quoteattr(dev_name)
             + ' audioInputDeviceName="" audioDeviceRate="48000.0" audioDeviceBufferSize="480"/>')
    text = text.replace("</PROPERTIES>",
                        '<VALUE name="audioDeviceState" val=' + quoteattr(state) + "/>\n</PROPERTIES>", 1)
    settings.write_text(text, encoding="utf-8")


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--exe", default=None)
    p.add_argument("--env", required=True, help="comma-separated WFS_TEST_* variables to set to 1")
    p.add_argument("--timeout", type=float, default=240.0)
    p.add_argument("--device", default=None, help='"type|name" of an audio device to open')
    args = p.parse_args()

    exe = common.find_exe(args.exe)
    env_vars = [v for v in args.env.split(",") if v]

    appdata = Path(os.environ["APPDATA"]) / "WFS-DIY"
    settings = appdata / "WFS-DIY.settings"
    log_dir = appdata / "logs"
    backup = Path(os.environ.get("TEMP", ".")) / "wfs-selftest-settings.bak"
    shutil.copy2(settings, backup)

    work_root = Path(os.environ.get("TEMP", ".")) / "wfs-control-replay" / "selftest"
    project = common.copy_fixture_to_temp(work_root)

    lines: list[str] = []
    app = None
    try:
        if args.device:
            inject_device(settings, args.device)
        # Only a log file this launch creates: the previous run's file can
        # still be the newest one for a second after its app closed.
        logs_before = set(log_dir.glob("WFS-DIY_*.log"))
        for v in env_vars:
            os.environ[v] = "1"

        common.kill_stale_instances()
        app = common.App(exe, common.fixture_wfs(project), ai_enabled=False)

        log = None
        deadline = time.monotonic() + args.timeout
        while time.monotonic() < deadline:
            if not app.alive():
                print(f"[selftest] the app exited early (code {app.proc.returncode})")
                break
            fresh = [f for f in log_dir.glob("WFS-DIY_*.log") if f not in logs_before]
            if fresh:
                log = max(fresh, key=lambda f: f.stat().st_mtime)
                if "SELF-TEST RESULT" in log.read_text(encoding="utf-8", errors="replace"):
                    break
            time.sleep(0.5)
        time.sleep(1.0)

        if log is not None:
            text = log.read_text(encoding="utf-8", errors="replace")
            lines = [ln.split("] ", 1)[-1] for ln in text.splitlines() if "SELF-TEST" in ln]
            print(f"[selftest] log: {log}")
    finally:
        if app is not None and not app.close():
            print("[selftest] WARNING: the app did not close gracefully", file=sys.stderr)
        for v in env_vars:
            os.environ.pop(v, None)
        shutil.copy2(backup, settings)
        shutil.rmtree(work_root, ignore_errors=True)

    for ln in lines:
        print(ln)
    return common.EXIT_PASS if any("SELF-TEST RESULT: ALL PASS" in ln for ln in lines) \
        else common.EXIT_MISMATCH


if __name__ == "__main__":
    raise SystemExit(main())
