"""Mock Android tablet for the Remote protocol (v2) — end-to-end checks.

Impersonates the WFS Control tablet over UDP against a live app instance and
asserts the v2 remote-protocol contract:

  1. handshake     /remote/ping arrives as ",ii" (seq, version 2)
  2. dump shape    after pong: /remote/dumpBegin first, /remote/stateComplete
                   last with a matching dump seq; every channel 1..N (channel 1
                   included) has a name + position; the selected-channel
                   detailed block (~95 msgs for channel 1) precedes the marker
  3. race          a gesture blast while a full dump is being collected/sent
                   must not swallow the dump (regression for the old
                   incomingProtocol early-return in sendMessagesAsBundles)
  4. resync        /remote/requestResync {1,5} resends those channels;
                   an empty requestResync yields a full dump with a fresh
                   dumpBegin seq (same code path as the project-load re-dump)

Stdlib-only, follows the control-replay harness conventions (common.py).
Exit codes: 0 pass, 1 mismatch, 2 usage, 3 app failed to start.

Usage:
  python remote_tablet_mock.py [--exe <path-to-WFS-DIY.exe>] [--keep-temp]
"""

from __future__ import annotations

import argparse
import shutil
import socket
import struct
import sys
import tempfile
import threading
import time
from pathlib import Path

from common import (EXIT_MISMATCH, EXIT_PASS, App, OSCSender,
                    copy_fixture_to_temp, find_exe, fixture_wfs,
                    kill_stale_instances)

EXPECTED_PROTOCOL_VERSION = 2
MOCK_LISTEN_PORT = 9020        # the Remote target's networkTSport in the fixture
APP_RX_PORT = 8000             # networkRxUDPport in the fixture


# ---------------------------------------------------------------------------
# Minimal OSC decoding (messages + #bundle), big-endian per spec
# ---------------------------------------------------------------------------

def _read_padded_string(data: bytes, pos: int) -> tuple[str, int]:
    end = data.index(b"\x00", pos)
    s = data[pos:end].decode("utf-8", errors="replace")
    pos = (end + 4) & ~3
    return s, pos


def decode_packet(data: bytes) -> list[tuple[str, str, list]]:
    """Flatten a datagram into [(address, typetags, args), ...]."""
    out: list[tuple[str, str, list]] = []
    if data.startswith(b"#bundle\x00"):
        pos = 16  # header + timetag
        while pos + 4 <= len(data):
            (size,) = struct.unpack(">i", data[pos:pos + 4])
            pos += 4
            if size <= 0 or pos + size > len(data):
                break
            out.extend(decode_packet(data[pos:pos + size]))
            pos += size
        return out
    try:
        address, pos = _read_padded_string(data, 0)
        if not address.startswith("/"):
            return out
        typetags = ""
        args: list = []
        if pos < len(data) and data[pos:pos + 1] == b",":
            typetags, pos = _read_padded_string(data, pos)
            for tag in typetags[1:]:
                if tag == "i":
                    (v,) = struct.unpack(">i", data[pos:pos + 4]); pos += 4
                elif tag == "f":
                    (v,) = struct.unpack(">f", data[pos:pos + 4]); pos += 4
                elif tag == "s":
                    v, pos = _read_padded_string(data, pos)
                else:
                    return out  # unknown tag — bail on this message
                args.append(v)
        out.append((address, typetags, args))
    except (ValueError, struct.error, IndexError):
        pass
    return out


# ---------------------------------------------------------------------------
# Mock tablet: receive loop with auto ping/heartbeat replies
# ---------------------------------------------------------------------------

class MockTablet:
    def __init__(self, listen_port: int = MOCK_LISTEN_PORT):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 262144)
        self.sock.bind(("127.0.0.1", listen_port))
        self.sock.settimeout(0.2)
        self.tx = OSCSender(port=APP_RX_PORT, delay=0.0)
        self.messages: list[tuple[str, str, list]] = []
        self.lock = threading.Lock()
        self.ping_typetags: str | None = None
        self.ping_version: int | None = None
        self.auto_pong = True
        self._stop = threading.Event()
        self.thread = threading.Thread(target=self._run, daemon=True)
        self.thread.start()

    def _run(self) -> None:
        while not self._stop.is_set():
            try:
                data, _addr = self.sock.recvfrom(65536)
            except socket.timeout:
                continue
            except OSError:
                break
            for address, typetags, args in decode_packet(data):
                with self.lock:
                    self.messages.append((address, typetags, args))
                if address == "/remote/ping" and args:
                    if self.ping_typetags is None:
                        self.ping_typetags = typetags
                        self.ping_version = args[1] if len(args) >= 2 else 1
                    if self.auto_pong:
                        self.tx.send("/remote/pong",
                                     [("i", args[0]),
                                      ("i", EXPECTED_PROTOCOL_VERSION)])
                elif address == "/remote/heartbeat" and args:
                    self.tx.send("/remote/heartbeatAck", [("i", args[0])])

    # -- queries over the accumulated message log ---------------------------

    def snapshot(self) -> list[tuple[str, str, list]]:
        with self.lock:
            return list(self.messages)

    def mark(self) -> int:
        with self.lock:
            return len(self.messages)

    def since(self, mark: int) -> list[tuple[str, str, list]]:
        with self.lock:
            return list(self.messages[mark:])

    def wait_for(self, predicate, timeout: float, mark: int = 0):
        """Poll the log until predicate(messages_since_mark) returns a truthy
        value; None on timeout."""
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            result = predicate(self.since(mark))
            if result:
                return result
            time.sleep(0.1)
        return None

    def close(self) -> None:
        self._stop.set()
        self.thread.join(2.0)
        self.sock.close()
        self.tx.close()


# ---------------------------------------------------------------------------
# Fixture prep: add a Remote target pointing at the mock
# ---------------------------------------------------------------------------

def add_remote_target(project_dir: Path) -> None:
    net = project_dir / "network.xml"
    text = net.read_text(encoding="utf-8")
    marker = ('networkTSProtocol="1" networkTSqlabPatch="1"/>')
    insert = (marker + '\n'
              '      <Target id="1" networkTSname="mock-tablet" '
              'networkTSdataMode="0" networkTSip="127.0.0.1" '
              f'networkTSport="{MOCK_LISTEN_PORT}" networkTSrxEnable="1" '
              'networkTStxEnable="1" networkTSProtocol="2" '
              'networkTSqlabPatch="1"/>')
    if marker not in text:
        print("[remote-mock] fixture network.xml layout changed; "
              "cannot insert Remote target", file=sys.stderr)
        raise SystemExit(EXIT_MISMATCH)
    net.write_text(text.replace(marker, insert, 1), encoding="utf-8")


# ---------------------------------------------------------------------------
# Dump analysis helpers
# ---------------------------------------------------------------------------

def find_dump(messages, after_seq: int | None = None):
    """Locate a complete dumpBegin..stateComplete cycle. Returns dict or None."""
    begin = None
    for i, (address, _tt, args) in enumerate(messages):
        if address == "/remote/dumpBegin" and len(args) >= 2:
            if after_seq is not None and args[0] <= after_seq:
                continue
            begin = (i, args[0], args[1])
        elif address == "/remote/stateComplete" and len(args) >= 2 and begin:
            bi, bseq, bcount = begin
            if args[1] == bseq:
                return {"seq": bseq, "count_begin": bcount,
                        "count_complete": args[0],
                        "body": messages[bi + 1:i]}
    return None


def channel_of(args: list) -> int | None:
    return args[0] if args and isinstance(args[0], int) else None


def analyze_dump(body) -> dict:
    names, positions = set(), set()
    per_channel_addresses: dict[int, set] = {}
    for address, _tt, args in body:
        ch = channel_of(args)
        if ch is None:
            continue
        if address == "/remoteInput/inputName":
            names.add(ch)
        elif address in ("/remoteInput/positionXY", "/remoteInput/positionX",
                         "/remoteInput/positionY", "/marker/positionXY"):
            positions.add(ch)
        if address.startswith("/remoteInput/"):
            per_channel_addresses.setdefault(ch, set()).add(address)
    return {"names": names, "positions": positions,
            "per_channel": per_channel_addresses}


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe")
    ap.add_argument("--keep-temp", action="store_true")
    opts = ap.parse_args()

    exe = find_exe(opts.exe)
    failures: list[str] = []

    def check(cond: bool, label: str) -> bool:
        print(("[remote-mock] PASS " if cond else "[remote-mock] FAIL ") + label)
        if not cond:
            failures.append(label)
        return cond

    kill_stale_instances()
    work_root = Path(tempfile.mkdtemp(prefix="wfs-remote-mock-"))
    project_dir = copy_fixture_to_temp(work_root / "work")
    add_remote_target(project_dir)

    tablet = MockTablet()
    app = App(exe, fixture_wfs(project_dir))
    try:
        app.wait_for_mcp()

        # ---- 1. handshake ------------------------------------------------
        got_ping = tablet.wait_for(
            lambda msgs: tablet.ping_typetags is not None, timeout=30.0)
        check(got_ping is not None, "ping received")
        if got_ping:
            check(tablet.ping_typetags == ",ii",
                  f"ping typetags ,ii (got {tablet.ping_typetags})")
            check(tablet.ping_version == EXPECTED_PROTOCOL_VERSION,
                  f"ping version {EXPECTED_PROTOCOL_VERSION} "
                  f"(got {tablet.ping_version})")

        # ---- 2. connect-time dump shape ---------------------------------
        dump = tablet.wait_for(lambda msgs: find_dump(msgs), timeout=30.0)
        check(dump is not None, "connect dump (dumpBegin..stateComplete)")
        if dump:
            n = dump["count_complete"]
            check(n == dump["count_begin"],
                  f"dumpBegin/stateComplete channel count agree ({n})")
            info = analyze_dump(dump["body"])
            missing_names = [c for c in range(1, n + 1)
                             if c not in info["names"]]
            missing_pos = [c for c in range(1, n + 1)
                           if c not in info["positions"]]
            check(not missing_names,
                  f"all {n} channels have a name (missing: {missing_names})")
            check(not missing_pos,
                  f"all {n} channels have a position (missing: {missing_pos})")
            ch1 = len(info["per_channel"].get(1, set()))
            check(ch1 >= 40,
                  f"channel 1 selected-channel detailed block present "
                  f"({ch1} distinct addresses)")

        last_seq = dump["seq"] if dump else -1

        # ---- 3. race: gesture blast during a full re-dump ---------------
        mark = tablet.mark()
        blast_stop = threading.Event()

        def blast() -> None:
            gesture = OSCSender(port=APP_RX_PORT, delay=0.0)
            x = 0.0
            while not blast_stop.is_set():
                x = (x + 0.01) % 2.0
                gesture.send("/remoteInput/positionXY",
                             [("i", 2), ("f", 1.0 + x), ("f", 2.0)])
                time.sleep(0.005)  # ~200 Hz
            gesture.close()

        blaster = threading.Thread(target=blast, daemon=True)
        blaster.start()
        time.sleep(0.3)  # gestures already in flight when the dump starts
        tablet.tx.send("/remote/requestResync", [])
        dump2 = tablet.wait_for(
            lambda msgs: find_dump(msgs, after_seq=last_seq),
            timeout=20.0, mark=mark)
        blast_stop.set()
        blaster.join(2.0)
        check(dump2 is not None,
              "full dump survives concurrent gesture blast (race regression)")
        if dump2:
            check(dump2["seq"] > last_seq,
                  f"re-dump has fresh seq ({dump2['seq']} > {last_seq})")
            last_seq = dump2["seq"]

        # ---- 4. per-channel resync --------------------------------------
        mark = tablet.mark()
        tablet.tx.send("/remote/requestResync", [("i", 1), ("i", 5)])

        def got_channels(msgs):
            names = {channel_of(a) for adr, _t, a in msgs
                     if adr == "/remoteInput/inputName"}
            return {1, 5} <= names
        check(tablet.wait_for(got_channels, timeout=15.0, mark=mark) is not None,
              "requestResync {1,5} resends channels 1 and 5")

        return EXIT_PASS if not failures else EXIT_MISMATCH
    finally:
        graceful = app.close()
        tablet.close()
        if not opts.keep_temp and graceful and not failures:
            shutil.rmtree(work_root, ignore_errors=True)
        else:
            print(f"[remote-mock] temp kept at {work_root}")
        if failures:
            print(f"[remote-mock] {len(failures)} failure(s):", file=sys.stderr)
            for f in failures:
                print(f"  - {f}", file=sys.stderr)


if __name__ == "__main__":
    sys.exit(main())
