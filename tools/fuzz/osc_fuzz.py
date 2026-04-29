"""WFS-DIY OSC fuzzer — external, stdlib-only, no app changes.

Run against a live WFS-DIY instance: send malformed OSC packets, then check
three signals after each batch:
  1. Process liveness (the app PID must still be running)
  2. New WFSLogger warnings/errors (tail the active log file)
  3. Optional read-back: GET <oscquery>/<path>?VALUE and assert finite

Findings (anything beyond "still alive, no new warnings, all read-backs
finite") are written to findings/<timestamp>/summary.csv.

Usage:
  python osc_fuzz.py --host 127.0.0.1 --udp 9000 --pid 12345 \\
                     --log "C:/Users/me/AppData/Roaming/WFS-DIY/logs" \\
                     --layer 1
  python osc_fuzz.py --layer all --query 8080 --channel 1
  python osc_fuzz.py --layer 3 --duration 60

Detection is best-effort: if --pid / --log / --query are omitted, the
relevant signal is skipped and noted in the summary.
"""

from __future__ import annotations

import argparse
import csv
import ctypes
import datetime as dt
import json
import math
import os
import random
import socket
import struct
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

import addresses
import corpus
from osc_codec import encode_message, encode_typed


# ---------------------------------------------------------------------------
# Process liveness — Win32 + POSIX
# ---------------------------------------------------------------------------

def is_alive(pid: int) -> bool:
    if pid <= 0:
        return True  # caller didn't supply a PID; treat as "unknown alive"
    if sys.platform == "win32":
        PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
        STILL_ACTIVE = 259
        kernel32 = ctypes.windll.kernel32
        h = kernel32.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, False, pid)
        if not h:
            return False
        ec = ctypes.c_ulong()
        ok = kernel32.GetExitCodeProcess(h, ctypes.byref(ec))
        kernel32.CloseHandle(h)
        return bool(ok) and ec.value == STILL_ACTIVE
    try:
        os.kill(pid, 0)
        return True
    except (OSError, ProcessLookupError):
        return False


# ---------------------------------------------------------------------------
# Log tailing — find newest WFS-DIY_*.log under the configured dir
# ---------------------------------------------------------------------------

@dataclass
class LogTail:
    log_dir: Optional[Path]
    active_path: Optional[Path] = None
    last_size: int = 0

    def find_active(self) -> Optional[Path]:
        if self.log_dir is None or not self.log_dir.is_dir():
            return None
        candidates = sorted(
            self.log_dir.glob("WFS-DIY_*.log"),
            key=lambda p: p.stat().st_mtime,
            reverse=True,
        )
        return candidates[0] if candidates else None

    def baseline(self) -> None:
        self.active_path = self.find_active()
        if self.active_path is not None and self.active_path.exists():
            self.last_size = self.active_path.stat().st_size
        else:
            self.last_size = 0

    def read_delta(self) -> str:
        """Return new bytes appended since the last baseline call."""
        path = self.find_active()
        if path is None or not path.exists():
            return ""
        # Log rotated since baseline — start fresh from byte 0.
        if path != self.active_path:
            self.active_path = path
            self.last_size = 0
        size = path.stat().st_size
        if size <= self.last_size:
            self.last_size = size
            return ""
        try:
            with open(path, "rb") as f:
                f.seek(self.last_size)
                data = f.read(size - self.last_size)
        except OSError as exc:
            return f"<<log read failed: {exc}>>"
        self.last_size = size
        try:
            return data.decode("utf-8", errors="replace")
        except Exception:
            return data.decode("latin-1", errors="replace")


WARN_MARKERS = ("[WARNING]", "[WARN]", "[ERROR]", "[CRITICAL]", "[FATAL]")


def scan_log_delta(text: str) -> list[str]:
    """Return only the lines that match a known severity marker."""
    if not text:
        return []
    return [line for line in text.splitlines()
            if any(m in line for m in WARN_MARKERS)]


# ---------------------------------------------------------------------------
# OSCQuery read-back — GET http://host:port/<path>?VALUE
# ---------------------------------------------------------------------------

@dataclass
class QueryClient:
    host: str
    port: Optional[int]

    def get_value(self, path: str, channel: int) -> tuple[bool, object]:
        """Returns (ok, value). ok=False means the GET failed; that's not
        itself a finding."""
        if self.port is None:
            return False, None
        # OSCQuery indexes per-channel sub-trees as /wfs/input/<n>/...
        path = path.format(ch=channel)
        url = f"http://{self.host}:{self.port}{path}?VALUE"
        try:
            with urllib.request.urlopen(url, timeout=1.5) as resp:
                body = resp.read().decode("utf-8", errors="replace")
        except (urllib.error.URLError, socket.timeout, ConnectionError):
            return False, None
        try:
            data = json.loads(body)
        except json.JSONDecodeError:
            return True, body
        # OSCQuery returns {"VALUE": [...]} typically
        if isinstance(data, dict) and "VALUE" in data:
            return True, data["VALUE"]
        return True, data


def is_non_finite(v) -> bool:
    if isinstance(v, float):
        return not math.isfinite(v)
    if isinstance(v, list):
        return any(is_non_finite(x) for x in v)
    if isinstance(v, str):
        if v.lower() in ("nan", "inf", "-inf", "infinity", "-infinity"):
            return True
    return False


# ---------------------------------------------------------------------------
# Sender — UDP datagram or TCP length-prefixed
# ---------------------------------------------------------------------------

# WFS-DIY's TCP receiver caps a single OSC packet at 65536 bytes (see
# OSCTCPReceiver.h::maxPacketSize). Anything larger gets dropped and the
# connection is closed.
TCP_MAX_PACKET = 65536


@dataclass
class Sender:
    host: str
    port: int
    use_tcp: bool = False
    delay: float = 0.005
    sock: Optional[socket.socket] = None
    connect_failures: int = 0
    max_connect_failures: int = 3

    def __post_init__(self) -> None:
        if self.use_tcp:
            self._reconnect_tcp()
        else:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def _reconnect_tcp(self) -> bool:
        if self.sock is not None:
            try:
                self.sock.close()
            except OSError:
                pass
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(1.0)
        try:
            self.sock.connect((self.host, self.port))
            self.connect_failures = 0
            return True
        except OSError as exc:
            self.connect_failures += 1
            if self.connect_failures <= self.max_connect_failures:
                print(f"[sender] tcp connect failed ({self.connect_failures}/"
                      f"{self.max_connect_failures}): {exc}", file=sys.stderr)
            return False

    def is_dead(self) -> bool:
        return self.use_tcp and self.connect_failures > self.max_connect_failures

    def send_raw(self, payload: bytes) -> None:
        if self.is_dead():
            return
        if self.use_tcp:
            # TCP framing: 4-byte big-endian size prefix + payload.
            # Empty or oversized packets are dropped client-side rather
            # than being sent — they'd just terminate the TCP connection
            # on the receiver, ending the run prematurely.
            if len(payload) == 0 or len(payload) > TCP_MAX_PACKET:
                return
            try:
                framed = struct.pack(">I", len(payload)) + payload
                self.sock.sendall(framed)
            except OSError as exc:
                print(f"[sender] tcp send failed: {exc} — reconnecting",
                      file=sys.stderr)
                self._reconnect_tcp()
        else:
            try:
                self.sock.sendto(payload, (self.host, self.port))
            except OSError as exc:
                print(f"[sender] sendto failed: {exc}", file=sys.stderr)
        if self.delay > 0:
            time.sleep(self.delay)

    def close(self) -> None:
        if self.sock is not None:
            try:
                self.sock.close()
            except OSError:
                pass


# ---------------------------------------------------------------------------
# Findings sink
# ---------------------------------------------------------------------------

@dataclass
class Finding:
    layer: str
    category: str
    address: str
    payload_summary: str
    signal: str          # 'crash' | 'log' | 'readback' | 'send-failed'
    detail: str
    expect: str = ""


@dataclass
class FindingSink:
    out_dir: Path
    findings: list[Finding] = field(default_factory=list)
    sent: int = 0

    def add(self, f: Finding) -> None:
        self.findings.append(f)
        # Echo to stdout so the user sees progress live.
        print(f"  [{f.signal:8}] {f.address} | {f.category} | {f.detail[:120]}")

    def write_summary(self) -> Path:
        self.out_dir.mkdir(parents=True, exist_ok=True)
        path = self.out_dir / "summary.csv"
        with open(path, "w", newline="", encoding="utf-8") as f:
            w = csv.writer(f)
            w.writerow(["layer", "signal", "category", "address",
                        "expect", "payload", "detail"])
            for x in self.findings:
                w.writerow([x.layer, x.signal, x.category, x.address,
                            x.expect, x.payload_summary, x.detail])
        return path


# ---------------------------------------------------------------------------
# Detection wrapper — run after each packet (or each small batch)
# ---------------------------------------------------------------------------

def detect(label: str, layer: str, category: str, address: str,
           payload_summary: str, expect: str,
           pid: int, log_tail: LogTail, query: QueryClient,
           readback_path: Optional[str], channel: int,
           sink: FindingSink) -> bool:
    """Returns True if the app is still alive. Records any findings."""
    # 1) Process liveness
    if not is_alive(pid):
        sink.add(Finding(layer, category, address, payload_summary,
                         "crash", f"app PID {pid} no longer running",
                         expect))
        return False

    # 2) Log tail — flag any new warning/error/critical
    delta = log_tail.read_delta()
    flagged = scan_log_delta(delta)
    for line in flagged[:5]:        # cap per-batch noise
        sink.add(Finding(layer, category, address, payload_summary,
                         "log", line.strip(), expect))

    # 3) Read-back — flag any non-finite value
    if readback_path is not None:
        ok, value = query.get_value(readback_path, channel)
        if ok and is_non_finite(value):
            sink.add(Finding(layer, category, address, payload_summary,
                             "readback",
                             f"non-finite value at {readback_path}: {value!r}",
                             expect))

    return True


# ---------------------------------------------------------------------------
# Layer 1 — corpus
# ---------------------------------------------------------------------------

def summarize_args(args: list[tuple[str, object]]) -> str:
    parts = []
    for tag, val in args:
        if isinstance(val, (bytes, bytearray)):
            parts.append(f"{tag}=<{len(val)}B>")
        elif isinstance(val, str) and len(val) > 32:
            parts.append(f"{tag}=str[{len(val)}]")
        else:
            parts.append(f"{tag}={val!r}")
    return ",".join(parts)


def run_layer1(sender: Sender, pid: int, log_tail: LogTail,
               query: QueryClient, channel: int,
               sink: FindingSink) -> bool:
    print(f"[layer1] sending {len(corpus.CASES)} corpus cases + "
          f"{len(corpus.RAW_PACKETS)} raw packets")
    for case in corpus.CASES:
        try:
            payload = encode_message(case.address, case.args)
        except Exception as exc:
            sink.add(Finding("1", case.category, case.address,
                             summarize_args(case.args), "send-failed",
                             f"encode error: {exc}", case.expect))
            continue
        sender.send_raw(payload)
        sink.sent += 1
        if not detect("layer1", "1", case.category, case.address,
                      summarize_args(case.args), case.expect,
                      pid, log_tail, query,
                      _readback_for(case.address), channel, sink):
            return False

    for category, raw, expect, notes in corpus.RAW_PACKETS:
        sender.send_raw(raw)
        sink.sent += 1
        if not detect("layer1", "1", category, "<raw>",
                      f"raw[{len(raw)}B]", expect,
                      pid, log_tail, query, None, channel, sink):
            return False
    return True


def _readback_for(address: str) -> Optional[str]:
    """Best-effort: map a write address to an OSCQuery read path."""
    if address.startswith("/wfs/input/"):
        param = address[len("/wfs/input/"):].split("/")[0]
        if param:
            return f"/wfs/input/{{ch}}/{param}"
    if address.startswith("/wfs/output/"):
        param = address[len("/wfs/output/"):].split("/")[0]
        if param:
            return f"/wfs/output/{{ch}}/{param}"
    if address.startswith("/wfs/reverb/"):
        param = address[len("/wfs/reverb/"):].split("/")[0]
        if param:
            return f"/wfs/reverb/{{ch}}/{param}"
    if address.startswith("/wfs/config/"):
        return address  # already an absolute path
    return None


# ---------------------------------------------------------------------------
# Layer 2 — per-address structured fuzz
# ---------------------------------------------------------------------------

def _build_packet(entry: addresses.AddrEntry, channel: int, value) -> bytes:
    if entry.shape == "channel-float":
        return encode_message(entry.address,
                              [("i", channel), ("f", float(value))])
    if entry.shape == "channel-string":
        return encode_message(entry.address,
                              [("i", channel), ("s", str(value))])
    if entry.shape == "channel-int":
        return encode_message(entry.address,
                              [("i", channel), ("i", int(value))])
    if entry.shape == "config-float":
        return encode_message(entry.address, [("f", float(value))])
    if entry.shape == "cluster-move":
        v = float(value)
        return encode_message(entry.address,
                              [("i", channel), ("f", v), ("f", v)])
    if entry.shape == "cluster-scalar":
        return encode_message(entry.address,
                              [("i", channel), ("f", float(value))])
    raise ValueError(f"unknown shape {entry.shape}")


def run_layer2(sender: Sender, pid: int, log_tail: LogTail,
               query: QueryClient, channel: int,
               sink: FindingSink) -> bool:
    print(f"[layer2] iterating {len(addresses.ENTRIES)} curated addresses")
    for entry in addresses.ENTRIES:
        if entry.value_kind == "name":
            pool = addresses.STRING_EDGES
        elif entry.value_kind == "bool" or entry.shape == "channel-int":
            pool = addresses.ints_for_kind(entry.value_kind)
        else:
            pool = addresses.floats_for_kind(entry.value_kind)

        for value in pool:
            try:
                payload = _build_packet(entry, channel, value)
            except Exception as exc:
                sink.add(Finding("2", "encode", entry.address,
                                 f"value={value!r}", "send-failed",
                                 f"encode error: {exc}", ""))
                continue
            sender.send_raw(payload)
            sink.sent += 1
            if not detect("layer2", "2", f"{entry.value_kind}",
                          entry.address, f"value={value!r}",
                          "varies",
                          pid, log_tail, query,
                          entry.read_back, channel, sink):
                return False
    return True


# ---------------------------------------------------------------------------
# Layer 3 — random walk
# ---------------------------------------------------------------------------

RAND_PREFIXES = [
    "/wfs/input/", "/wfs/output/", "/wfs/reverb/", "/wfs/config/",
    "/remoteInput/", "/cluster/", "/adm/obj/", "/arrayAdjust/", "/random/",
]
RAND_PARAMS = [
    "positionX", "positionY", "positionZ", "attenuation", "name", "x", "y",
    "freq", "gain", "totallyFakeParam", "", "../../etc",
]
RAND_TAGS = ["i", "f", "s", "T", "F", "N", "d", "h", "b"]


def _rand_value(tag: str, rng: random.Random):
    if tag == "i":
        return rng.choice([rng.randint(-1000, 1000),
                           -1, 0, 1, 2**31 - 1, -(2**31)])
    if tag == "f":
        return rng.choice([rng.uniform(-1000, 1000),
                           float("nan"), float("inf"), float("-inf"),
                           0.0, 1e30, -1e30])
    if tag == "s":
        n = rng.choice([0, 1, 8, 64, 1024])
        return "".join(chr(rng.randint(32, 126)) for _ in range(n))
    if tag == "b":
        return bytes(rng.randint(0, 255) for _ in range(rng.randint(0, 64)))
    if tag == "d":
        return rng.uniform(-1e6, 1e6)
    if tag == "h":
        return rng.randint(-(2**62), 2**62)
    return None


def run_layer3(sender: Sender, pid: int, log_tail: LogTail,
               query: QueryClient, channel: int,
               sink: FindingSink, duration: float) -> bool:
    rng = random.Random(0xCAFEBABE)
    end = time.monotonic() + duration
    print(f"[layer3] random walk for {duration:.1f}s")
    while time.monotonic() < end:
        prefix = rng.choice(RAND_PREFIXES)
        param = rng.choice(RAND_PARAMS)
        address = prefix + param
        n_args = rng.randint(0, 6)
        args = []
        for _ in range(n_args):
            tag = rng.choice(RAND_TAGS)
            args.append((tag, _rand_value(tag, rng)))
        try:
            payload = encode_message(address, args)
        except Exception:
            continue
        sender.send_raw(payload)
        sink.sent += 1
        # Detection every 50 packets — random walk is high-volume, the
        # per-packet detection cost would dominate.
        if sink.sent % 50 == 0:
            if not detect("layer3", "3", "random", address,
                          f"{n_args} args", "ignored",
                          pid, log_tail, query, None, channel, sink):
                return False
    return True


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--host", default="127.0.0.1")
    p.add_argument("--udp", type=int, default=None,
                   help="UDP port WFS-DIY listens on (visible in System "
                        "Config tab). Required unless --tcp is set.")
    p.add_argument("--tcp", type=int, default=None,
                   help="TCP port WFS-DIY listens on (default 8001). "
                        "When set, the fuzzer uses TCP with length-prefixed "
                        "framing. Mutually exclusive with --udp.")
    p.add_argument("--query", type=int, default=None,
                   help="OSCQuery HTTP port for read-back. Omit to skip.")
    p.add_argument("--pid", type=int, default=0,
                   help="WFS-DIY process PID for liveness checks. Omit "
                        "to skip.")
    p.add_argument("--log", type=Path, default=None,
                   help="Directory containing WFS-DIY_*.log files. Omit "
                        "to skip log scanning.")
    p.add_argument("--channel", type=int, default=1,
                   help="Channel index used for per-channel writes and "
                        "read-backs (1-based)")
    p.add_argument("--layer", choices=["1", "2", "3", "all"], default="1")
    p.add_argument("--duration", type=float, default=30.0,
                   help="Layer 3 wall-clock duration in seconds")
    p.add_argument("--delay", type=float, default=0.005,
                   help="Inter-packet delay in seconds")
    p.add_argument("--out", type=Path, default=None,
                   help="Output directory for findings. Defaults to "
                        "tools/fuzz/findings/<timestamp>")
    args = p.parse_args()

    if (args.udp is None) == (args.tcp is None):
        p.error("specify exactly one of --udp or --tcp")

    ts = dt.datetime.now().strftime("%Y%m%d-%H%M%S")
    out_dir = args.out or (Path(__file__).parent / "findings" / ts)

    use_tcp = args.tcp is not None
    target_port = args.tcp if use_tcp else args.udp
    sender = Sender(args.host, target_port, use_tcp=use_tcp, delay=args.delay)

    if use_tcp and sender.connect_failures > 0:
        print(f"[fuzz] !!! initial TCP connect to {args.host}:{target_port} "
              f"failed — aborting (is the app running and TCP listener on?)")
        return 3
    log_tail = LogTail(args.log)
    log_tail.baseline()
    query = QueryClient(args.host, args.query)
    sink = FindingSink(out_dir)

    if args.log is not None:
        if not args.log.is_dir():
            print(f"[fuzz] WARNING: --log path does not exist: {args.log}")
            print(f"[fuzz]   WFS-DIY writes to %APPDATA%/WFS-DIY/logs by default")
            print(f"[fuzz]   try: --log \"C:/Users/{os.environ.get('USERNAME', 'YOU')}/AppData/Roaming/WFS-DIY/logs\"")
        elif log_tail.active_path is None:
            print(f"[fuzz] WARNING: --log {args.log} contains no WFS-DIY_*.log files")
            print(f"[fuzz]   log scanning will be inactive for this run")

    transport = "TCP" if use_tcp else "UDP"
    print(f"[fuzz] target={args.host}:{target_port} ({transport}) "
          f"pid={args.pid or 'n/a'} "
          f"query={args.query or 'n/a'} log={log_tail.active_path or 'n/a'}")
    print(f"[fuzz] writing findings to {out_dir}")

    layers = ["1", "2", "3"] if args.layer == "all" else [args.layer]
    alive = True
    for layer in layers:
        if not alive:
            break
        if layer == "1":
            alive = run_layer1(sender, args.pid, log_tail, query,
                               args.channel, sink)
        elif layer == "2":
            alive = run_layer2(sender, args.pid, log_tail, query,
                               args.channel, sink)
        elif layer == "3":
            alive = run_layer3(sender, args.pid, log_tail, query,
                               args.channel, sink, args.duration)

    sender.close()
    summary = sink.write_summary()
    print(f"\n[fuzz] sent {sink.sent} packets, "
          f"{len(sink.findings)} findings")
    print(f"[fuzz] summary: {summary}")
    if not alive:
        print("[fuzz] !!! app died during run — see summary for last batch")
        return 2
    return 0 if not sink.findings else 1


if __name__ == "__main__":
    raise SystemExit(main())
