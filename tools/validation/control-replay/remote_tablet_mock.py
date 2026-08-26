"""Mock Android tablet for the Remote protocol (v4) — end-to-end checks.

Impersonates the WFS Control tablet over UDP against a live app instance and
asserts the remote-protocol contract:

  1. handshake     /remote/ping arrives as ",ii" (seq, version 4)
  2. dump shape    after pong: /remote/dumpBegin first, /remote/stateComplete
                   last with a matching dump seq; the dump carries a
                   /remote/channelList inventory whose numbers are EXACTLY
                   the channels the body names — no more, no less — and every
                   one of them has a name + position; the selected-channel
                   detailed block (~95 msgs for channel 1) precedes the marker
  3. race          a gesture blast while a full dump is being collected/sent
                   must not swallow the dump (regression for the old
                   incomingProtocol early-return in sendMessagesAsBundles)
  4. resync        /remote/requestResync {1,5} resends those channels;
                   an empty requestResync yields a full dump with a fresh
                   dumpBegin seq (same code path as the project-load re-dump)
  5. shared load   the fixture is patched into an "old project": cluster 1 in
                   Shared Position mode with members 1+2 at diverged positions;
                   load-time invariant enforcement must dump them coincident
  6. release echo  after a /cluster/positionXY drag, the final
                   /remoteInput/positionXY release write must be echoed back
                   for EVERY member (release coords differ from the last drag
                   step, so only the release echo can carry them)
  7. vis mirror    v3 /remote/vis/* contract: config + outputArrays +
                   selection + a delays/levels row pair for the selected
                   channel arrive after connect; /remote/vis/pin N answers
                   with channel N's rows without triggering a channel dump;
                   moving a source refreshes rows at <= ~10 Hz
  8. inventory     v4 /remote/channelList: ",i…i" = live count N followed by
                   N interleaved (number, isStereo) pairs in DISPLAY order,
                   a full-replacement snapshot carrying no sequence number.
                   Arity, ranges and number uniqueness are validated, and the
                   fixture is loaded with two channel nodes swapped so the
                   numbers are NOT in ascending order — the count stopped
                   being an enumeration and 1..N is no longer a valid guess.
                   One node is typed stereo, so a flag half hardcoded to 0
                   fails here instead of silently hiding the Stereo Width /
                   Stereo Axis dials on every pair. Both are patched into the
                   project on disk because neither is drivable live:
                   moveInputChannel and the mono/stereo type have no OSC
                   address and no MCP tool, only the channel-list dialog
  9. structural    a delete over MCP retires a permanent number: a standalone
                   /remote/channelList replaces the inventory, the gap it
                   leaves survives into the next full dump, and the highest
                   live number now runs past the live count

Stdlib-only, follows the control-replay harness conventions (common.py).
Exit codes: 0 pass, 1 mismatch, 2 usage, 3 app failed to start.

Usage:
  python remote_tablet_mock.py [--exe <path-to-WFS-DIY.exe>] [--keep-temp]
"""

from __future__ import annotations

import argparse
import re
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
                    kill_stale_instances, tool_payload)

EXPECTED_PROTOCOL_VERSION = 4
MOCK_LISTEN_PORT = 9020        # the Remote target's networkTSport in the fixture
APP_RX_PORT = 8000             # networkRxUDPport in the fixture
MAX_CHANNEL_NUMBER = 64        # WFSParameterDefaults::maxInputChannels

# Fixture channels swapped on disk to make the display order non-ascending,
# the one typed stereo, and the channel deleted at the end. The swap may not
# touch 1 or 2 (the shared-cluster members, picked out of inputs.xml by
# document order); the delete may not touch 1, 2 or 5 (the vis pin target),
# which every later check still addresses by number; the stereo channel must
# survive the delete, so it may not be DELETED_CHANNEL.
REORDER_SWAP = (5, 7)
STEREO_CHANNEL = 7
DELETED_CHANNEL = 3


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


def make_diverged_shared_cluster(project_dir: Path) -> None:
    """Patch the temp fixture into an 'old project': cluster 1 becomes Shared
    Position (mode 2) and inputs 1+2 join it while keeping their diverged
    fixture positions (-2.0, 1.5) and (2.0, 1.5) — the state an older app
    version could have saved before the invariant existed."""
    system = project_dir / "system.xml"
    text = system.read_text(encoding="utf-8")
    marker = '<Cluster id="1" clusterReferenceMode="0"'
    if marker not in text:
        print("[remote-mock] fixture system.xml layout changed; "
              "cannot set cluster 1 to Shared Position", file=sys.stderr)
        raise SystemExit(EXIT_MISMATCH)
    system.write_text(
        text.replace(marker, '<Cluster id="1" clusterReferenceMode="2"', 1),
        encoding="utf-8")

    inputs = project_dir / "inputs.xml"
    text = inputs.read_text(encoding="utf-8")
    if text.count('inputCluster="0"') < 2:
        print("[remote-mock] fixture inputs.xml layout changed; "
              "cannot assign inputs 1+2 to cluster 1", file=sys.stderr)
        raise SystemExit(EXIT_MISMATCH)
    # First two occurrences belong to inputs 1 and 2 (document order).
    text = text.replace('inputCluster="0"', 'inputCluster="1"', 2)
    inputs.write_text(text, encoding="utf-8")


_INPUT_NODE_RE = re.compile(r'[ \t]*<Input id="(\d+)">.*?</Input>\r?\n', re.S)


def reorder_input_channels(project_dir: Path, a: int, b: int) -> list[int]:
    """Swap two <Input> nodes in the temp fixture and return the resulting
    display order. Tree order IS the display order while the permanent numbers
    stay put, so the loaded project's channel list is not in ascending order —
    the state a desktop drag-reorder saves.

    Patched on disk because nothing can drive a live reorder: moveInputChannel
    is reachable only from the channel-list dialog, with no OSC address and no
    MCP tool behind it.

    Must run AFTER make_diverged_shared_cluster, which picks inputs 1 and 2 out
    of inputs.xml by document order — so neither of them may be swapped."""
    inputs = project_dir / "inputs.xml"
    text = inputs.read_text(encoding="utf-8")
    nodes = list(_INPUT_NODE_RE.finditer(text))
    order = [int(m.group(1)) for m in nodes]
    if a in (1, 2) or b in (1, 2) or a not in order or b not in order:
        print(f"[remote-mock] cannot swap inputs {a} and {b}: not both live "
              f"outside the cluster-1 pair (inputs.xml order {order})",
              file=sys.stderr)
        raise SystemExit(EXIT_MISMATCH)

    ia, ib = order.index(a), order.index(b)
    first, second = sorted((nodes[ia], nodes[ib]), key=lambda m: m.start())
    text = (text[:first.start()] + second.group(0)
            + text[first.end():second.start()] + first.group(0)
            + text[second.end():])
    inputs.write_text(text, encoding="utf-8")

    order[ia], order[ib] = b, a
    return order


def make_stereo_input_channel(project_dir: Path, number: int) -> None:
    """Type one <Input> node stereo in the temp fixture. An all-mono fixture
    cannot tell a correct isStereo flag from one hardcoded to 0, and the flag is
    the tablet's only source of truth for showing the Stereo Width / Stereo Axis
    dials and the picker's pair badge.

    A partially typed list is post-rework data: migrateInputChannelModel defaults
    the untyped nodes to mono rather than falling back to the legacy tail split.

    Patched on disk for the same reason as the reorder — the type is settable
    only from the channel-list dialog. Must run AFTER reorder_input_channels,
    whose node regex matches `<Input id="N">` with no other attributes."""
    inputs = project_dir / "inputs.xml"
    text = inputs.read_text(encoding="utf-8")
    marker = f'<Input id="{number}">'
    if marker not in text:
        print(f'[remote-mock] fixture inputs.xml has no <Input id="{number}">; '
              "cannot type it stereo", file=sys.stderr)
        raise SystemExit(EXIT_MISMATCH)
    text = text.replace(
        marker, f'<Input id="{number}" inputChannelType="stereo">', 1)
    inputs.write_text(text, encoding="utf-8")


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


def parse_channel_list(args: list) -> list[tuple[int, int]] | None:
    """Decode /remote/channelList — arg 0 is the live channel count N, then N
    interleaved (number, isStereo) pairs IN DISPLAY ORDER. Returns
    [(number, is_stereo), ...] or None when the payload is malformed.

    Arity, ranges and number uniqueness are all rejected here rather than
    half-applied: the message is a full-replacement snapshot with no sequence
    number, so a receiver that keeps its previous inventory on None degrades to
    stale, while one that applies a truncated list loses channels outright."""
    if not args or not all(isinstance(a, int) for a in args):
        return None
    n = args[0]
    if n < 0 or len(args) != 1 + 2 * n:
        return None
    entries: list[tuple[int, int]] = []
    seen: set[int] = set()
    for k in range(n):
        number, is_stereo = args[1 + 2 * k], args[2 + 2 * k]
        if not 1 <= number <= MAX_CHANNEL_NUMBER:
            return None
        if is_stereo not in (0, 1) or number in seen:
            return None
        seen.add(number)
        entries.append((number, is_stereo))
    return entries


def latest_channel_list(messages):
    """(entries, raw_args) of the LAST /remote/channelList in `messages` —
    last-one-wins, like the receiver. entries is None when that payload failed
    validation; both are None when the message never arrived."""
    raw = None
    for address, _tt, args in messages:
        if address == "/remote/channelList":
            raw = args
    if raw is None:
        return None, None
    return parse_channel_list(raw), raw


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
    make_diverged_shared_cluster(project_dir)
    display_order = reorder_input_channels(project_dir, *REORDER_SWAP)
    make_stereo_input_channel(project_dir, STEREO_CHANNEL)
    print(f"[remote-mock] fixture display order {display_order}, "
          f"channel {STEREO_CHANNEL} stereo")

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

            # The inventory, not the count, says which channels exist: numbers
            # are permanent, so they carry gaps and are not sorted. Expanding
            # range(1, n + 1) here is exactly the bug this message exists to
            # kill — it demands channels that were deleted and hides channels
            # numbered above the count.
            inventory, raw_list = latest_channel_list(dump["body"])
            check(raw_list is not None, "dump carries a /remote/channelList")
            check(raw_list is None or inventory is not None,
                  f"channelList payload validates (1 + 2N ints, numbers "
                  f"1..{MAX_CHANNEL_NUMBER} unique, isStereo in 0/1): "
                  f"{raw_list}")
            if inventory is not None:
                numbers = [c for c, _s in inventory]
                check(len(inventory) == raw_list[0] == n,
                      f"inventory length, its own arg 0 and the dump count "
                      f"agree ({len(inventory)} / {raw_list[0]} / {n})")
                check(numbers == display_order,
                      f"inventory is the fixture's display order, numbers not "
                      f"ascending (got {numbers}, expected {display_order})")
                # Both halves of every pair, or a flag stuck at 0 passes: the
                # tablet reads nothing else to decide a channel is a pair.
                # Keyed by number, not by position, so an order regression is
                # reported by the check above alone and not twice.
                flags = dict(inventory)
                check(flags.get(STEREO_CHANNEL) == 1
                      and all(s == 0 for c, s in flags.items()
                              if c != STEREO_CHANNEL),
                      f"isStereo is 1 for channel {STEREO_CHANNEL} and 0 for "
                      f"every mono channel ({inventory})")
                unlisted = sorted(info["names"] - set(numbers))
                unnamed = [c for c in numbers if c not in info["names"]]
                check(not unlisted and not unnamed,
                      f"inventory is exactly the set the dump names "
                      f"(named but not listed: {unlisted}, listed but not "
                      f"named: {unnamed})")
                missing_pos = [c for c in numbers if c not in info["positions"]]
                check(not missing_pos,
                      f"all {len(numbers)} listed channels have a position "
                      f"(missing: {missing_pos})")
            ch1 = len(info["per_channel"].get(1, set()))
            check(ch1 >= 40,
                  f"channel 1 selected-channel detailed block present "
                  f"({ch1} distinct addresses)")

        last_seq = dump["seq"] if dump else -1

        # ---- 5. shared-cluster load-time invariant enforcement ----------
        # The fixture was patched so cluster 1 is Shared Position with members
        # 1 (-2.0, 1.5) and 2 (2.0, 1.5) diverged on disk; load-time
        # enforcement must snap member 2 onto member 1. The connect dump can
        # interleave with the post-load re-dump (two detached paced senders),
        # so analyze a dedicated quiescent full resync instead — and do it
        # BEFORE the gesture checks below move the cluster.
        def last_position(msgs, ch):
            pos = None
            for adr, _tt, a in msgs:
                if adr == "/remoteInput/positionXY" and len(a) >= 3 and a[0] == ch:
                    pos = (a[1], a[2])
                elif adr == "/remoteInput/positionX" and len(a) >= 2 and a[0] == ch:
                    pos = (a[1], pos[1] if pos else None)
                elif adr == "/remoteInput/positionY" and len(a) >= 2 and a[0] == ch:
                    pos = (pos[0] if pos else None, a[1])
            return pos

        eps = 1e-3
        time.sleep(1.5)  # let any post-load re-dump drain first
        mark = tablet.mark()
        tablet.tx.send("/remote/requestResync", [])
        dump5 = tablet.wait_for(
            lambda msgs: find_dump(msgs, after_seq=last_seq),
            timeout=20.0, mark=mark)
        if dump5:
            last_seq = dump5["seq"]
            # v3: every full dump must embed the vis config with counts taken
            # from the actual channel trees (the Config/IO *properties* can
            # drift in sessions saved by older versions — a dump announcing
            # 0 outputs wedges the tablet on "waiting for data").
            dump_cfgs = [a for adr, _tt, a in dump5["body"]
                         if adr == "/remote/vis/config" and len(a) >= 2]
            check(bool(dump_cfgs) and dump_cfgs[-1][0] == 16,
                  f"resync dump embeds vis config with real output count "
                  f"({dump_cfgs})")
            p1 = last_position(dump5["body"], 1)
            p2 = last_position(dump5["body"], 2)
            coincident = (p1 is not None and p2 is not None and
                          None not in p1 and None not in p2 and
                          abs(p1[0] - (-2.0)) < eps and abs(p1[1] - 1.5) < eps and
                          abs(p2[0] - p1[0]) < eps and abs(p2[1] - p1[1]) < eps)
            check(coincident,
                  f"shared cluster loads coincident at (-2.0, 1.5) "
                  f"(ch1={p1}, ch2={p2})")
        else:
            check(False, "shared-cluster load check: no quiescent dump")

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

        # ---- 6. release write echoes the whole shared cluster ------------
        # Simulate the tablet's drag: throttled /cluster/positionXY steps,
        # then the single /remoteInput/positionXY release write at coords
        # that differ from the last drag step. Only the fix-B echo can carry
        # the release coords for the NON-dragged member (channel 2).
        mark = tablet.mark()
        for i in range(5):
            tablet.tx.send("/cluster/positionXY",
                           [("i", 1), ("f", -1.0 + 0.7 * i), ("f", 2.0 + 0.4 * i)])
            time.sleep(0.04)
        release = (3.0, 4.0)
        tablet.tx.send("/remoteInput/positionXY",
                       [("i", 1), ("f", release[0]), ("f", release[1])])

        def release_echoed(msgs):
            got1 = got2 = False
            for adr, _tt, a in msgs:
                if adr == "/remoteInput/positionXY" and len(a) >= 3:
                    at_release = (abs(a[1] - release[0]) < eps and
                                  abs(a[2] - release[1]) < eps)
                    if a[0] == 1 and at_release:
                        got1 = True
                    elif a[0] == 2 and at_release:
                        got2 = True
            return got1 and got2
        check(tablet.wait_for(release_echoed, timeout=10.0, mark=mark) is not None,
              f"release write echoed for both members at {release}")

        # ---- 7. v3 visualisation mirroring ------------------------------
        # 7a. connect-time init: config + outputArrays + selection + a
        # delays/levels pair for the selected channel, with row float counts
        # matching the announced channel counts.
        def find_vis_init(msgs):
            cfg = arrays = sel = delays = levels = None
            for adr, _tt, a in msgs:
                if adr == "/remote/vis/config" and len(a) >= 2:
                    cfg = (a[0], a[1])
                elif adr == "/remote/vis/outputArrays" and a:
                    arrays = a
                elif adr == "/remote/vis/selection" and len(a) >= 3:
                    sel = a
                elif adr == "/remote/vis/delays" and len(a) >= 3:
                    delays = a
                elif adr == "/remote/vis/levels" and len(a) >= 3:
                    levels = a
            if cfg and arrays and sel and delays and levels:
                return {"cfg": cfg, "arrays": arrays, "sel": sel,
                        "delays": delays, "levels": levels}
            return None

        vis = tablet.wait_for(lambda _msgs: find_vis_init(tablet.snapshot()),
                              timeout=10.0)
        check(vis is not None,
              "vis init (config + outputArrays + selection + row pair)")
        if vis:
            n_out, n_rev = vis["cfg"]
            check(n_out > 0, f"vis config numOutputs > 0 ({n_out})")
            check(vis["arrays"][0] == n_out and
                  len(vis["arrays"]) == 1 + n_out,
                  f"outputArrays carries {n_out} array ids")
            for label, row in (("delays", vis["delays"]),
                               ("levels", vis["levels"])):
                ch, ro, rr = row[0], row[1], row[2]
                check(ro == n_out and rr == n_rev and
                      len(row) == 3 + n_out + n_rev,
                      f"vis {label} row self-describes {n_out}+{n_rev} floats "
                      f"(ch {ch})")
            check(all(-60.0 <= v <= 0.0 for v in vis["levels"][3:]),
                  "vis levels are dB in [-60, 0]")

        # 7b. pin: rows for channel 5 arrive, no channel dump is triggered,
        # and the desktop selection (vis selection primary) is untouched.
        mark = tablet.mark()
        tablet.tx.send("/remote/vis/pin", [("i", 5)])

        def pinned_rows(msgs):
            got_d = got_l = False
            for adr, _tt, a in msgs:
                if adr == "/remote/vis/delays" and a and a[0] == 5:
                    got_d = True
                elif adr == "/remote/vis/levels" and a and a[0] == 5:
                    got_l = True
            return got_d and got_l
        check(tablet.wait_for(pinned_rows, timeout=10.0, mark=mark) is not None,
              "pin 5 answered with channel 5 delays+levels rows")
        time.sleep(1.0)
        pin_msgs = tablet.since(mark)
        check(not any(adr == "/remote/dumpBegin" for adr, _tt, _a in pin_msgs),
              "pin did not trigger a dump")
        check(not any(adr == "/remote/vis/selection" and a and a[0] == 5
                      for adr, _tt, a in pin_msgs),
              "pin did not move the desktop selection")

        # 7c. moving a source refreshes rows, throttled to <= ~10 Hz.
        mark = tablet.mark()
        drag = OSCSender(port=APP_RX_PORT, delay=0.0)
        drag_seconds = 1.5
        t0 = time.monotonic()
        x = 0.0
        while time.monotonic() - t0 < drag_seconds:
            x = (x + 0.02) % 2.0
            drag.send("/remoteInput/positionXY",
                      [("i", 1), ("f", -1.0 + x), ("f", 1.0)])
            time.sleep(0.01)  # ~100 Hz gesture
        drag.close()
        time.sleep(0.5)  # trailing-edge send
        moved = [a for adr, _tt, a in tablet.since(mark)
                 if adr == "/remote/vis/delays" and a and a[0] == 1]
        check(len(moved) >= 1, f"drag produced fresh vis rows ({len(moved)})")
        check(len(moved) <= int(drag_seconds * 10) + 4,
              f"vis rows throttled to <= ~10 Hz ({len(moved)} in "
              f"{drag_seconds}s)")
        cfg_repeats = [a for adr, _tt, a in tablet.since(mark)
                       if adr == "/remote/vis/config"]
        check(len(cfg_repeats) >= 1,
              f"vis config repeats with updates ({len(cfg_repeats)} during "
              "drag) - a tablet that lost the connect-time config recovers")

        # 7d. unpin stops pinned-channel rows.
        tablet.tx.send("/remote/vis/pin", [("i", 0)])
        time.sleep(0.3)
        mark = tablet.mark()
        drag2 = OSCSender(port=APP_RX_PORT, delay=0.0)
        for i in range(20):
            drag2.send("/remoteInput/positionXY",
                       [("i", 1), ("f", -0.5 + 0.02 * i), ("f", 1.2)])
            time.sleep(0.02)
        drag2.close()
        time.sleep(0.5)
        after_unpin = [a for adr, _tt, a in tablet.since(mark)
                       if adr == "/remote/vis/delays" and a and a[0] == 5]
        check(not after_unpin, "unpin stops channel 5 rows")

        # ---- 9. structural change: a delete retires a number -------------
        # Runs last: it changes the channel set every check above addresses.
        # Deleting a MIDDLE channel is the case a count cannot express — the
        # survivors keep their numbers, so the live set gains a permanent gap
        # and the highest number outruns the count.
        expected_after = [c for c in display_order if c != DELETED_CHANNEL]
        mark = tablet.mark()
        _, deletion = app.tool_confirmed("input_delete",
                                         {"input_id": DELETED_CHANNEL})
        payload = tool_payload(deletion)
        check(isinstance(payload, dict)
              and payload.get("deleted_channel_id") == DELETED_CHANNEL
              and payload.get("total") == len(expected_after),
              f"MCP input_delete retired channel {DELETED_CHANNEL} ({payload})")

        # A standalone push and a re-dump that embeds one are equally valid
        # deliveries — the snapshot is last-one-wins either way — so this only
        # requires that a fresh list arrives at all.
        pushed = tablet.wait_for(
            lambda msgs: latest_channel_list(msgs)[1] is not None,
            timeout=15.0, mark=mark)
        check(pushed is not None,
              "structural change delivers a fresh /remote/channelList")
        after, raw_after = latest_channel_list(tablet.since(mark))
        if raw_after is not None:
            check(after is not None,
                  f"pushed channelList payload validates ({raw_after})")
        if after is not None:
            after_numbers = [c for c, _s in after]
            check(after_numbers == expected_after,
                  f"inventory drops {DELETED_CHANNEL} and keeps the display "
                  f"order of the rest (got {after_numbers}, expected "
                  f"{expected_after})")
            check(sorted(after_numbers)
                  != list(range(1, len(after_numbers) + 1)),
                  f"live numbers are NOT dense 1..N - the retired number "
                  f"stays a gap ({after_numbers})")
            # The push builds its payload independently of the dump, so the
            # type has to be re-checked here and not only in section 2.
            check(dict(after).get(STEREO_CHANNEL) == 1,
                  f"pushed inventory still types channel {STEREO_CHANNEL} "
                  f"stereo ({after})")

        # The gap must survive a full dump too, not just the push: the tablet
        # rebuilds from whichever of the two arrives last.
        time.sleep(1.5)  # let the delete's own re-dump drain first
        mark = tablet.mark()
        tablet.tx.send("/remote/requestResync", [])
        dump9 = tablet.wait_for(
            lambda msgs: find_dump(msgs, after_seq=last_seq),
            timeout=20.0, mark=mark)
        check(dump9 is not None, "full dump after the delete")
        if dump9:
            info9 = analyze_dump(dump9["body"])
            inv9, raw9 = latest_channel_list(dump9["body"])
            check(inv9 is not None,
                  f"post-delete dump carries a valid channelList ({raw9})")
            if inv9 is not None:
                numbers9 = [c for c, _s in inv9]
                check(numbers9 == expected_after,
                      f"post-delete dump inventory matches the push (got "
                      f"{numbers9}, expected {expected_after})")
                check(len(inv9) == raw9[0] == dump9["count_complete"],
                      f"post-delete inventory length, its arg 0 and the dump "
                      f"count agree ({len(inv9)} / {raw9[0]} / "
                      f"{dump9['count_complete']})")
                check(set(numbers9) == info9["names"],
                      f"post-delete dump names exactly the listed channels "
                      f"(listed {sorted(numbers9)}, named "
                      f"{sorted(info9['names'])})")

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
