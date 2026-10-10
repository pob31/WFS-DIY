"""A stand-in for Go.dot, to bench WFS-DIY's Go.dot client without Go.dot.

Listens on Go.dot's OSC port and answers the two commands WFS-DIY sends, the way
Go.dot's docs/godot-authoring-protocol-0.1.md says Go.dot answers them:

  /godot/cmd/mount/declare  ,siis  prefix port queryPort name
      -> /godot/declared   ,ss   <device id> created|updated
      -> fetches GET http://<sender>:<queryPort><prefix>, checks it is an OSCQuery
         tree rooted at the prefix, and answers
         /godot/described  ,sis  <device id> <node count> <problem>
  /godot/cmd/cue/capture    ,sssssss[ss]...  where target id name number notes messageIds pairs
      -> /godot/captured   ,ssss <id> created|updated|appended|<reason> <number> <name>

Every captured address is checked against the description fetched at the last
declare: it must be a node, and its atom count must match the node's TYPE (what
Go.dot checks at GO). A mismatch is printed and answered `bad-address` or
`type-mismatch`, so WFS-DIY's status line shows the refusal a real Go.dot would
give. Answers go to the declared port, from this socket, as Go.dot does.

Stdlib only (repo convention). Not part of run_selftest: it waits for a human
to press "Write to Go.dot" in a running WFS-DIY.

Usage:
  python godot_mock.py [--port 8010] [--refuse locked] [--seconds 0]
Exit codes: 0 every capture matched the description, 1 a mismatch, 2 usage.
"""

from __future__ import annotations

import argparse
import json
import re
import socket
import struct
import sys
import time
import urllib.request

sys.stdout.reconfigure(encoding="utf-8")


# ---------------------------------------------------------------------------
# OSC
# ---------------------------------------------------------------------------

def _pad(b: bytes) -> bytes:
    return b + b"\0" * ((4 - len(b) % 4) % 4)


def osc_string(s: str) -> bytes:
    return _pad(s.encode("utf-8") + b"\0")


def osc_message(address: str, *args) -> bytes:
    tags, payload = ",", b""
    for a in args:
        if isinstance(a, int):
            tags += "i"
            payload += struct.pack(">i", a)
        elif isinstance(a, float):
            tags += "f"
            payload += struct.pack(">f", a)
        else:
            tags += "s"
            payload += osc_string(str(a))
    return osc_string(address) + osc_string(tags) + payload


def _read_string(data: bytes, pos: int):
    end = data.index(b"\0", pos)
    s = data[pos:end].decode("utf-8")
    return s, (end + 4) & ~3


def parse_message(data: bytes):
    address, pos = _read_string(data, 0)
    tags, pos = _read_string(data, pos)
    args = []
    for t in tags[1:]:
        if t == "i":
            args.append(struct.unpack(">i", data[pos:pos + 4])[0]); pos += 4
        elif t == "f":
            args.append(struct.unpack(">f", data[pos:pos + 4])[0]); pos += 4
        elif t == "s":
            s, pos = _read_string(data, pos); args.append(s)
        else:
            raise ValueError(f"unexpected type tag {t!r}")
    return address, args


# ---------------------------------------------------------------------------
# Atoms: the spelling of a Go.dot OSC cue's value row
# ---------------------------------------------------------------------------

ATOM = re.compile(r'\s*(s:"(?:[^"\\]|\\.)*"|[ihfd]:\S+|[TFNI])')


def atoms_of(text: str):
    """The atoms of a value list, or None when it does not read (Go.dot: bad-value)."""
    out, pos = [], 0
    text = text.strip()
    while pos < len(text):
        m = ATOM.match(text, pos)
        if not m:
            return None
        out.append(m.group(1))
        pos = m.end()
    return out


# ---------------------------------------------------------------------------
# The description
# ---------------------------------------------------------------------------

def flatten(node: dict, out: dict):
    path = node.get("FULL_PATH")
    is_leaf = "TYPE" in node or (node.get("ACCESS") in (2, 3) and "CONTENTS" not in node)
    if path and is_leaf:
        out[path] = node.get("TYPE", "")
    for child in (node.get("CONTENTS") or {}).values():
        if isinstance(child, dict):
            flatten(child, out)


def fetch_description(host: str, port: int, prefix: str):
    with urllib.request.urlopen(f"http://{host}:{port}{prefix}", timeout=5) as r:
        tree = json.loads(r.read().decode("utf-8"))
    if tree.get("FULL_PATH") != prefix:
        raise ValueError(f"the root's FULL_PATH is {tree.get('FULL_PATH')!r}, not {prefix!r}")
    nodes = {}
    flatten(tree, nodes)
    return nodes


# ---------------------------------------------------------------------------

def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--port", type=int, default=8010)
    ap.add_argument("--refuse", default="", help="answer every capture with this reason (e.g. locked)")
    ap.add_argument("--seconds", type=float, default=0, help="stop after this long (0: never)")
    args = ap.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", args.port))
    sock.settimeout(0.5)
    print(f"godot_mock: listening on UDP {args.port}")

    devices = {}      # prefix -> {"id", "host", "port", "nodes"}
    cues = {}         # id -> list of (address, atoms)
    mismatches = 0
    deadline = time.time() + args.seconds if args.seconds > 0 else None

    def device_for(host):
        for d in devices.values():
            if d["host"] == host:
                return d
        return None

    while deadline is None or time.time() < deadline:
        try:
            data, (host, _) = sock.recvfrom(65536)
        except socket.timeout:
            continue
        try:
            address, a = parse_message(data)
        except Exception as e:  # noqa: BLE001 - a bench tool says what it got
            print(f"  unreadable datagram from {host}: {e}")
            continue

        if address == "/godot/cmd/mount/declare" and len(a) >= 4:
            prefix, port, query_port, name = a[0], a[1], a[2], a[3]
            existing = devices.get(prefix)
            outcome = "updated" if existing else "created"
            dev = existing or {"id": "MCK%05d" % len(devices), "nodes": {}}
            dev.update(host=host, port=port)
            devices[prefix] = dev
            sock.sendto(osc_message("/godot/declared", dev["id"], outcome), (host, port))
            print(f"declare {prefix} from {host}:{port} query {query_port} name {name!r} -> {outcome}")
            if query_port:
                try:
                    dev["nodes"] = fetch_description(host, query_port, prefix)
                    problem = ""
                except Exception as e:  # noqa: BLE001
                    problem = str(e)
                sock.sendto(osc_message("/godot/described", dev["id"], len(dev["nodes"]), problem), (host, port))
                print(f"  described: {len(dev['nodes'])} nodes {problem}")
            continue

        if address == "/godot/cmd/cue/capture" and len(a) >= 7:
            where, target, cue_id, name, number, notes, _ids = a[:7]
            pairs = list(zip(a[7::2], a[8::2]))
            dev = device_for(host)
            reason = args.refuse
            if not reason and (len(a) - 7) % 2:
                reason = "bad-value"
            for addr, value in pairs:
                if reason:
                    break
                atoms = atoms_of(value)
                if atoms is None:
                    reason = "bad-value"
                elif dev and dev["nodes"]:
                    if addr not in dev["nodes"]:
                        reason = "bad-address"
                        print(f"  NOT IN THE DESCRIPTION: {addr} {value}")
                    elif len(dev["nodes"][addr]) != len(atoms):
                        reason = "type-mismatch"
                        print(f"  TYPE {dev['nodes'][addr]!r} vs {len(atoms)} atom(s): {addr} {value}")
            if reason:
                mismatches += reason in ("bad-address", "type-mismatch")
                outcome = reason
            elif where == "more":
                cues.setdefault(cue_id, []).extend(pairs)
                outcome = "appended"
            else:
                outcome = "updated" if cue_id in cues else "created"
                cues[cue_id] = list(pairs)
            reply_to = (host, dev["port"]) if dev else None
            if reply_to:
                sock.sendto(osc_message("/godot/captured", cue_id, outcome, number, name), reply_to)
            print(f"capture {where} {cue_id} {name!r}: {len(pairs)} pair(s) -> {outcome}"
                  + ("" if reply_to else " (no declared device at that host: no answer)"))
            continue

        print(f"  other: {address} {a}")

    return 1 if mismatches else 0


if __name__ == "__main__":
    sys.exit(main())
