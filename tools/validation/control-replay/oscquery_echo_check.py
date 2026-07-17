"""OSCQuery echo-suppression check.

Regression driver for the feedback loop that punched Ableton automation
out of playback: the app must NOT push an OSCQuery WebSocket value update
back to the client whose OSC message caused the change, while every OTHER
subscriber still receives it.

Two WebSocket clients subscribe (LISTEN) to the same paths from distinct
loopback source IPs (127.0.0.1 and 127.0.0.2 — both routable on the
loopback /8, no firewall involvement). UDP OSC writes are sent from each
source IP in turn; the sender's own client must stay silent and the other
client must see the pushes. Covered write paths: coalesced generic params,
"special" immediate params (positionX), polar conversion (positionR),
fade-time ramps (whole trajectory suppressed to the originator), and a
rapid multi-sender interleave.

Stdlib-only (repo convention). WebSocket client is a minimal RFC 6455
implementation: LISTEN is a masked text frame
`{"COMMAND":"LISTEN","DATA":"/wfs/input/1/positionX"}`, server pushes are
unmasked binary frames carrying one raw OSC message.

Usage:
  python oscquery_echo_check.py [--exe path] [--keep-temp]

Exit codes: 0 pass, 1 fail, 2 usage, 3 app failed to start.
"""

from __future__ import annotations

import argparse
import base64
import json
import os
import secrets
import shutil
import socket
import struct
import sys
import threading
import time
from pathlib import Path

import common

WS_HOST = "127.0.0.1"
SETTLE_AFTER_WRITE = 0.35   # > OSCIngestQueue drain + 30ms WS flush
IP_A = "127.0.0.1"
IP_B = "127.0.0.2"


# ---------------------------------------------------------------------------
# Minimal OSC message parser (server pushes: address + ",f|,i|,s" + one arg)
# ---------------------------------------------------------------------------

def parse_osc(data: bytes):
    def aligned(n: int) -> int:
        return (n + 4) & ~3

    end = data.index(b"\x00")
    address = data[:end].decode("utf-8", "replace")
    pos = aligned(end)
    tend = data.index(b"\x00", pos)
    typetag = data[pos:tend].decode("ascii", "replace")
    pos = aligned(tend)
    values = []
    for t in typetag.lstrip(","):
        if t == "f":
            values.append(struct.unpack(">f", data[pos:pos + 4])[0])
            pos += 4
        elif t == "i":
            values.append(struct.unpack(">i", data[pos:pos + 4])[0])
            pos += 4
        elif t == "s":
            send_ = data.index(b"\x00", pos)
            values.append(data[pos:send_].decode("utf-8", "replace"))
            pos = aligned(send_)
        else:
            break
    return address, values


# ---------------------------------------------------------------------------
# Minimal RFC 6455 WebSocket client
# ---------------------------------------------------------------------------

class WSClient:
    """WebSocket client with a fixed local source IP. Received binary
    frames are parsed as OSC and collected in .pushes as (address, values);
    text frames (PATH_CHANGED etc.) are ignored."""

    def __init__(self, name: str, source_ip: str,
                 host: str = WS_HOST, port: int = common.OSCQUERY_HTTP_PORT):
        self.name = name
        self.pushes: list[tuple[str, list]] = []
        self._lock = threading.Lock()
        self._sock = socket.create_connection(
            (host, port), timeout=5.0, source_address=(source_ip, 0))
        key = base64.b64encode(secrets.token_bytes(16)).decode("ascii")
        req = (f"GET / HTTP/1.1\r\n"
               f"Host: {host}:{port}\r\n"
               f"Upgrade: websocket\r\n"
               f"Connection: Upgrade\r\n"
               f"Sec-WebSocket-Key: {key}\r\n"
               f"Sec-WebSocket-Version: 13\r\n\r\n")
        self._sock.sendall(req.encode("ascii"))
        response = b""
        while b"\r\n\r\n" not in response:
            chunk = self._sock.recv(4096)
            if not chunk:
                raise ConnectionError(f"[{name}] WS handshake: socket closed")
            response += chunk
        status = response.split(b"\r\n", 1)[0].decode("ascii", "replace")
        if " 101 " not in status + " ":
            raise ConnectionError(f"[{name}] WS handshake refused: {status}")
        self._buf = response.split(b"\r\n\r\n", 1)[1]
        self._closing = False
        self._reader = threading.Thread(target=self._read_loop, daemon=True)
        self._reader.start()

    # -- send ---------------------------------------------------------------

    def _send_frame(self, opcode: int, payload: bytes) -> None:
        mask = secrets.token_bytes(4)
        header = bytes([0x80 | opcode])
        n = len(payload)
        if n < 126:
            header += bytes([0x80 | n])
        elif n < 65536:
            header += bytes([0x80 | 126]) + struct.pack(">H", n)
        else:
            header += bytes([0x80 | 127]) + struct.pack(">Q", n)
        masked = bytes(b ^ mask[i % 4] for i, b in enumerate(payload))
        self._sock.sendall(header + mask + masked)

    def listen(self, path: str) -> None:
        cmd = json.dumps({"COMMAND": "LISTEN", "DATA": path})
        self._send_frame(0x1, cmd.encode("utf-8"))

    # -- receive ------------------------------------------------------------

    def _recv_exact(self, n: int) -> bytes:
        while len(self._buf) < n:
            chunk = self._sock.recv(4096)
            if not chunk:
                raise ConnectionError("closed")
            self._buf += chunk
        out, self._buf = self._buf[:n], self._buf[n:]
        return out

    def _read_loop(self) -> None:
        try:
            while not self._closing:
                b0, b1 = self._recv_exact(2)
                opcode = b0 & 0x0F
                length = b1 & 0x7F
                if length == 126:
                    length = struct.unpack(">H", self._recv_exact(2))[0]
                elif length == 127:
                    length = struct.unpack(">Q", self._recv_exact(8))[0]
                if b1 & 0x80:  # masked server frame — nonstandard, unmask
                    mask = self._recv_exact(4)
                    payload = bytes(b ^ mask[i % 4] for i, b in
                                    enumerate(self._recv_exact(length)))
                else:
                    payload = self._recv_exact(length)
                if opcode == 0x2:      # binary: OSC value push
                    addr, values = parse_osc(payload)
                    with self._lock:
                        self.pushes.append((addr, values))
                elif opcode == 0x9:    # ping -> pong
                    self._send_frame(0xA, payload)
                elif opcode == 0x8:    # close
                    return
                # text (0x1) frames: PATH_CHANGED etc. — ignore
        except (ConnectionError, OSError):
            pass

    def snapshot(self) -> list[tuple[str, list]]:
        with self._lock:
            return list(self.pushes)

    def pushes_for(self, path: str) -> list[list]:
        return [v for a, v in self.snapshot() if a == path]

    def close(self) -> None:
        self._closing = True
        try:
            self._send_frame(0x8, b"")
        except OSError:
            pass
        try:
            self._sock.close()
        except OSError:
            pass


# ---------------------------------------------------------------------------
# Checks
# ---------------------------------------------------------------------------

PATHS = [
    "/wfs/input/4/attenuation",   # coalesced generic param
    "/wfs/input/1/positionX",     # "special" immediate param
    "/wfs/input/2/positionX",     # written via polar /wfs/input/positionR
    "/wfs/input/2/positionY",
    "/wfs/input/3/distanceAttenuation",   # fade-time ramp trajectory
    "/wfs/input/5/attenuation",   # interleave: written by sender A
    "/wfs/input/6/attenuation",   # interleave: written by sender B
]

failures: list[str] = []


def check(cond: bool, message: str) -> None:
    if cond:
        print(f"[echo-check] ok: {message}")
    else:
        failures.append(message)
        print(f"[echo-check] FAIL: {message}", file=sys.stderr)


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--exe", default=None)
    p.add_argument("--keep-temp", action="store_true")
    args = p.parse_args()

    exe = common.find_exe(args.exe)
    work_root = Path(os.environ.get("TEMP", ".")) / "wfs-control-replay" \
        / "oscquery_echo_check"
    project = common.copy_fixture_to_temp(work_root)

    common.kill_stale_instances()
    app = common.App(exe, common.fixture_wfs(project), ai_enabled=False)
    client_a = client_b = None
    try:
        app.wait_for_mcp()
        app.wait_for_oscquery()

        client_a = WSClient("A", IP_A)
        client_b = WSClient("B", IP_B)
        for path in PATHS:
            client_a.listen(path)
            client_b.listen(path)
        time.sleep(0.5)  # let LISTEN commands land

        # UDP senders with distinct source IPs. Loopback source selection
        # follows the bind address, so bind explicitly.
        sender_a = common.OSCSender(delay=0.0)
        sender_a.sock.bind((IP_A, 0))
        sender_b = common.OSCSender(delay=0.0)
        sender_b.sock.bind((IP_B, 0))

        # --- single-sender legs (all from A = 127.0.0.1) -------------------
        sender_a.send("/wfs/input/attenuation", [("i", 4), ("f", -12.5)])
        time.sleep(SETTLE_AFTER_WRITE)
        sender_a.send("/wfs/input/positionX", [("i", 1), ("f", -3.25)])
        time.sleep(SETTLE_AFTER_WRITE)
        sender_a.send("/wfs/input/positionR", [("i", 2), ("f", 2.0)])
        time.sleep(SETTLE_AFTER_WRITE)
        # fade-time ramp: 0.5 s trajectory to -2.5 dB/m (distanceAttenuation
        # is in the ramp-capable whitelist; plain attenuation is not)
        sender_a.send("/wfs/input/distanceAttenuation",
                      [("i", 3), ("f", -2.5), ("f", 0.5)])
        time.sleep(1.2)  # ramp completes + final flush

        # --- multi-sender interleave (A -> ch5, B -> ch6, rapid) -----------
        for i in range(5):
            sender_a.send("/wfs/input/attenuation", [("i", 5), ("f", -7.0 - i)])
            sender_b.send("/wfs/input/attenuation", [("i", 6), ("f", -8.0 - i)])
            time.sleep(0.02)
        time.sleep(1.0)
        sender_a.close()
        sender_b.close()

        # --- state actually changed (HTTP read-back) -----------------------
        check(common.oscquery_get("/wfs/input/4/attenuation") == [-12.5],
              "attenuation ch4 applied")
        check(common.oscquery_get("/wfs/input/1/positionX") == [-3.25],
              "positionX ch1 applied")
        att3 = common.oscquery_get("/wfs/input/3/distanceAttenuation")
        check(isinstance(att3, list) and abs(att3[0] - (-2.5)) < 1e-3,
              f"ramp ch3 reached target (got {att3})")

        # --- echo suppression: the sender's own client stays silent --------
        for path in ("/wfs/input/4/attenuation", "/wfs/input/1/positionX",
                     "/wfs/input/2/positionX", "/wfs/input/2/positionY",
                     "/wfs/input/3/distanceAttenuation",
                     "/wfs/input/5/attenuation"):
            got = client_a.pushes_for(path)
            check(not got, f"client A (sender) got NO echo for {path} "
                           f"(got {len(got)} pushes)")
        got_b6 = client_b.pushes_for("/wfs/input/6/attenuation")
        check(not got_b6, "client B (sender of ch6) got NO echo for "
                          f"/wfs/input/6/attenuation (got {len(got_b6)})")

        # --- positive control: the OTHER client receives every change ------
        check(client_b.pushes_for("/wfs/input/4/attenuation") == [[-12.5]],
              "client B received coalesced push ch4")
        check(client_b.pushes_for("/wfs/input/1/positionX") == [[-3.25]],
              "client B received special push ch1")
        check(len(client_b.pushes_for("/wfs/input/2/positionX")) >= 1,
              "client B received polar-derived positionX ch2")
        ramp_pushes = client_b.pushes_for("/wfs/input/3/distanceAttenuation")
        check(len(ramp_pushes) >= 2
              and abs(ramp_pushes[-1][0] - (-2.5)) < 1e-3,
              f"client B received ramp trajectory ch3 "
              f"({len(ramp_pushes)} pushes, last {ramp_pushes[-1] if ramp_pushes else None})")
        b5 = client_b.pushes_for("/wfs/input/5/attenuation")
        check(len(b5) >= 1 and abs(b5[-1][0] - (-11.0)) < 1e-3,
              f"client B received interleaved ch5 writes from A (last {b5[-1] if b5 else None})")
        a6 = client_a.pushes_for("/wfs/input/6/attenuation")
        check(len(a6) >= 1 and abs(a6[-1][0] - (-12.0)) < 1e-3,
              f"client A received interleaved ch6 writes from B (last {a6[-1] if a6 else None})")
    finally:
        for c in (client_a, client_b):
            if c is not None:
                c.close()
        app.close()

    if not args.keep_temp:
        shutil.rmtree(work_root, ignore_errors=True)

    if failures:
        print(f"[echo-check] {len(failures)} check(s) FAILED", file=sys.stderr)
        return common.EXIT_MISMATCH
    print("[echo-check] PASS")
    return common.EXIT_PASS


if __name__ == "__main__":
    raise SystemExit(main())
