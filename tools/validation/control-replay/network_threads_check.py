"""Network thread checks (re-audit 2026-09-29, N3, N4, N5).

Run A - the fixture with OSC tracking on (protocol 1, UDP 7000):

  N3, TCP: 20 clients (--tcp-clients) connect to the OSC TCP port and close without sending.
      Each left its handler thread spinning at 100 % CPU (JUCE's non-blocking
      read returns 0 at end of stream, and a closed socket stays readable) and
      never inactive, so from the 17th on every client was refused. Asserted:
      the app's CPU over the next 3 s stays low, and a 21st client's write lands.
  N5: 60 000 tracking datagrams in one burst. Each was one callAsync holding the
      receiver's raw `this`, with no bound. Asserted: the burst goes through
      the bounded tracking queue (its drop report reaches the session log) and
      an ordinary write sent after it still lands.
  N4: OSCQuery GETs from 4 threads while 5 000 OSC writes arrive. The reply
      used to be built on the HTTP server's threads, racing every write. A
      smoke check: every GET answers 200 and the app stays up (the race it
      removes was not reproducible on demand).

Run B - MQTT tracking pointed at a local fake broker that completes the
handshake and then closes the connection cleanly:

  N3, MQTT: the receive loop took waitUntilReady's -1 as "ready" and ignored a
      0-byte read, so after a clean close it spun at 100 % CPU and never
      reached its reconnect. Asserted: at least 3 connections within 10 s
      (reconnects back off 1 s, 2 s) and low CPU.

Usage:
  python network_threads_check.py [--exe path] [--keep-temp]

Exit codes: 0 pass, 1 fail, 2 usage, 3 app failed to start.
"""

from __future__ import annotations

import argparse
import ctypes
import ctypes.wintypes
import os
import re
import shutil
import socket
import struct
import sys
import threading
import time
import urllib.error
import urllib.request
from pathlib import Path

import common
from osc_codec import encode_message

TCP_PORT = 8001          # networkRxTCPport in the fixture
TRACKING_PORT = 7000     # trackingPort in the fixture
MQTT_PORT = 18831
CPU_MARGIN = 0.6         # cores above rest; a spinning thread is a whole core

# ---------------------------------------------------------------------------


class _FileTime(ctypes.Structure):
    _fields_ = [("low", ctypes.wintypes.DWORD), ("high", ctypes.wintypes.DWORD)]


def process_cpu_seconds(pid: int) -> float:
    kernel32 = ctypes.windll.kernel32
    handle = kernel32.OpenProcess(0x1000, False, pid)   # PROCESS_QUERY_LIMITED_INFORMATION
    if not handle:
        raise OSError("OpenProcess failed")
    try:
        c, e, k, u = _FileTime(), _FileTime(), _FileTime(), _FileTime()
        if not kernel32.GetProcessTimes(handle, ctypes.byref(c), ctypes.byref(e),
                                        ctypes.byref(k), ctypes.byref(u)):
            raise OSError("GetProcessTimes failed")
        ticks = ((k.high << 32) | k.low) + ((u.high << 32) | u.low)
        return ticks / 1e7
    finally:
        kernel32.CloseHandle(handle)


def cpu_cores_over(pid: int, seconds: float) -> float:
    before, t0 = process_cpu_seconds(pid), time.monotonic()
    time.sleep(seconds)
    return (process_cpu_seconds(pid) - before) / (time.monotonic() - t0)


def stage_width_lands(value: float, timeout: float = 10.0) -> float | None:
    """Send the stage width over UDP; seconds until OSCQuery reads it back."""
    sender = common.OSCSender(delay=0.0)
    t0 = time.monotonic()
    sender.send("/wfs/config/stage/width", [("f", value)])
    sender.close()
    while time.monotonic() - t0 < timeout:
        try:
            if common.oscquery_get("/wfs/config/stage/width") == [value]:
                return time.monotonic() - t0
        except Exception:  # noqa: BLE001 - keep polling
            pass
        time.sleep(0.2)
    return None


def newest_session_log(after: float) -> Path | None:
    logs = Path(os.environ["APPDATA"]) / "WFS-DIY" / "logs"
    found = [p for p in logs.glob("WFS-DIY_*.log") if p.stat().st_mtime >= after]
    return max(found, key=lambda p: p.stat().st_mtime) if found else None


def patch_network(project: Path, protocol: int, extra: dict[str, str] | None = None) -> None:
    network = project / "network.xml"
    text = network.read_text(encoding="utf-8")
    text = text.replace('trackingEnabled="0"', 'trackingEnabled="1"', 1)
    text = re.sub(r'trackingProtocol="\d+"', f'trackingProtocol="{protocol}"', text, count=1)
    for key, value in (extra or {}).items():
        text = re.sub(key + r'="[^"]*"', f'{key}="{value}"', text, count=1)
    network.write_text(text, encoding="utf-8")


def enable_tracking_on_first_inputs(project: Path) -> None:
    inputs = project / "inputs.xml"
    parts = re.split(r'(<Input\b)', inputs.read_text(encoding="utf-8"))
    for i in range(1, min(len(parts), 5), 2):
        parts[i + 1] = parts[i + 1].replace('inputTrackingActive="0"', 'inputTrackingActive="1"', 1)
    inputs.write_text("".join(parts), encoding="utf-8")


# ---------------------------------------------------------------------------


def run_a(exe: Path, work_root: Path, failures: list[str], tcp_clients: int) -> None:
    project = common.copy_fixture_to_temp(work_root / "a")
    patch_network(project, protocol=1)
    enable_tracking_on_first_inputs(project)

    started = time.time() - 1.0
    common.kill_stale_instances()
    app = common.App(exe, common.fixture_wfs(project), ai_enabled=False)
    try:
        app.wait_for_oscquery()
        time.sleep(1.5)
        pid = app.proc.pid

        idle = cpu_cores_over(pid, 3.0)
        print(f"[net] app CPU at rest: {idle:.2f} cores")

        # ---- N3, TCP ----------------------------------------------------------
        for _ in range(tcp_clients):
            s = socket.create_connection(("127.0.0.1", TCP_PORT), timeout=5)
            s.close()
            time.sleep(0.05)
        time.sleep(1.0)
        after_closes = cpu_cores_over(pid, 3.0)
        print(f"[net] app CPU after {tcp_clients} TCP clients closed: {after_closes:.2f} cores")
        if after_closes > idle + CPU_MARGIN:
            failures.append(f"after {tcp_clients} TCP clients closed cleanly the app burns {after_closes:.1f} "
                            f"cores (at rest {idle:.1f}): their handler threads spin (re-audit N3)")

        client = socket.create_connection(("127.0.0.1", TCP_PORT), timeout=5)
        packet = encode_message("/wfs/config/stage/width", [("f", 17.0)])
        client.sendall(struct.pack(">I", len(packet)) + packet)
        landed = None
        t0 = time.monotonic()
        while time.monotonic() - t0 < 5.0:
            try:
                if common.oscquery_get("/wfs/config/stage/width") == [17.0]:
                    landed = time.monotonic() - t0
                    break
            except Exception:  # noqa: BLE001
                pass
            time.sleep(0.2)
        client.close()
        if landed is None:
            failures.append(f"a TCP client after {tcp_clients} closed ones could not write: the "
                            "closed clients still hold every slot (re-audit N3)")

        # ---- N4: GETs racing writes ------------------------------------------
        get_results: list[object] = []
        stop = threading.Event()

        def getter():
            while not stop.is_set():
                try:
                    with urllib.request.urlopen(
                            f"http://127.0.0.1:{common.OSCQUERY_HTTP_PORT}/wfs/input/1/positionX?VALUE",
                            timeout=10) as r:
                        get_results.append(r.status)
                except urllib.error.HTTPError as exc:
                    get_results.append(exc.code)
                except Exception as exc:  # noqa: BLE001
                    get_results.append(repr(exc))

        getters = [threading.Thread(target=getter) for _ in range(4)]
        for g in getters:
            g.start()
        writer = common.OSCSender(delay=0.0)
        for k in range(5000):
            writer.send("/wfs/input/positionX", [("i", 1 + k % 4), ("f", (k % 200) / 10.0 - 10.0)])
            if k % 250 == 0:
                time.sleep(0.01)
        writer.close()
        time.sleep(1.0)
        stop.set()
        for g in getters:
            g.join()
        bad = [r for r in get_results if r != 200]
        print(f"[net] OSCQuery GETs during the write storm: {len(get_results)}, not 200: {len(bad)}")
        if not app.alive():
            failures.append("the app died while OSCQuery GETs raced OSC writes (re-audit N4)")
        elif not get_results or bad:
            failures.append(f"OSCQuery GETs during OSC writes: {len(bad)} of {len(get_results)} "
                            f"did not answer 200 ({bad[:3]}) (re-audit N4)")

        # ---- N5: tracking burst ----------------------------------------------
        burst = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        for k in range(60000):
            burst.sendto(encode_message("/wfs/tracking",
                                        [("i", 1 + k % 2), ("f", (k % 100) / 50.0), ("f", 0.5), ("f", 0.0)]),
                         ("127.0.0.1", TRACKING_PORT))
        burst.close()
        after_burst = stage_width_lands(19.0, timeout=15.0)
        print(f"[net] an ordinary write after the tracking burst landed in: {after_burst}")
        if after_burst is None:
            failures.append("after a 60 000-datagram tracking burst an ordinary write did not land "
                            "in 15 s (re-audit N5)")
        if not app.alive():
            failures.append("the app died during the tracking burst (re-audit N5)")
    finally:
        app.close()

    log = newest_session_log(started)
    text = log.read_text(encoding="utf-8", errors="replace") if log else ""
    if "Tracking OSC queue full" not in text:
        failures.append("the tracking burst left no 'Tracking OSC queue full' line: tracking OSC "
                        "does not go through a bounded queue (re-audit N5)")


# ---------------------------------------------------------------------------


class FakeBroker(threading.Thread):
    """Accept, answer CONNECT and SUBSCRIBE, then close cleanly: every time."""

    def __init__(self, port: int):
        super().__init__(daemon=True)
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server.bind(("127.0.0.1", port))
        self.server.listen(8)
        self.server.settimeout(0.5)
        self.connections: list[float] = []
        self.stopping = threading.Event()

    def run(self):
        while not self.stopping.is_set():
            try:
                conn, _ = self.server.accept()
            except (socket.timeout, OSError):
                continue
            self.connections.append(time.monotonic())
            try:
                conn.settimeout(3.0)
                conn.recv(1024)                          # CONNECT
                conn.sendall(bytes([0x20, 0x02, 0x00, 0x00]))
                sub = conn.recv(1024)                    # SUBSCRIBE
                pid = sub[2:4] if len(sub) >= 4 else b"\x00\x01"
                conn.sendall(bytes([0x90, 0x03]) + pid + bytes([0x00]))
                time.sleep(0.3)
            except OSError:
                pass
            finally:
                conn.close()                            # the clean close under test

    def stop(self):
        self.stopping.set()
        self.server.close()


def run_b(exe: Path, work_root: Path, failures: list[str]) -> None:
    project = common.copy_fixture_to_temp(work_root / "b")
    patch_network(project, protocol=4, extra={"trackingMqttHost": "127.0.0.1",
                                              "trackingPort": str(MQTT_PORT)})
    broker = FakeBroker(MQTT_PORT)
    broker.start()

    common.kill_stale_instances()
    app = common.App(exe, common.fixture_wfs(project), ai_enabled=False)
    try:
        app.wait_for_oscquery()
        t0 = time.monotonic()
        while not broker.connections and time.monotonic() - t0 < 15.0:
            time.sleep(0.2)
        if not broker.connections:
            failures.append("the MQTT tracking receiver never connected to the local broker")
            return
        cores = cpu_cores_over(app.proc.pid, 10.0)
        count = len(broker.connections)
        print(f"[net] MQTT: {count} connection(s) in about 10 s, app CPU {cores:.2f} cores")
        if count < 3:
            failures.append(f"after the broker closed cleanly the MQTT receiver reconnected only "
                            f"{count - 1} time(s) in 10 s: its loop never left (re-audit N3)")
        if cores > CPU_MARGIN:   # the app at rest is about 0.1 core
            failures.append(f"with the broker closing cleanly the app burns {cores:.1f} cores (re-audit N3)")
    finally:
        app.close()
        broker.stop()


# ---------------------------------------------------------------------------


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--exe", default=None)
    p.add_argument("--keep-temp", action="store_true")
    p.add_argument("--tcp-clients", type=int, default=20,
                   help="clients that connect and close; 17+ also fills every slot on a "
                        "build that leaks them (each leaked one spins a core, so a "
                        "negative control can use fewer)")
    args = p.parse_args()

    exe = common.find_exe(args.exe)
    work_root = Path(os.environ.get("TEMP", ".")) / "wfs-control-replay" / "network_threads"

    failures: list[str] = []
    run_a(exe, work_root, failures, args.tcp_clients)
    time.sleep(3.0)   # single instance: let the first run exit
    run_b(exe, work_root, failures)

    if not args.keep_temp:
        shutil.rmtree(work_root, ignore_errors=True)

    for failure in failures:
        print(f"[net] HARD FAIL: {failure}", file=sys.stderr)
    if not failures:
        print("[net] ALL PASS")
    return common.EXIT_PASS if not failures else common.EXIT_MISMATCH


if __name__ == "__main__":
    raise SystemExit(main())
