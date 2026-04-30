"""Minimal OSC encoder/decoder for the fuzzer.

Hand-rolled so we can produce intentionally malformed packets that python-osc
would refuse to emit. Stdlib only.

Type tag conventions used here:
  i  int32
  f  float32
  s  OSC-string (null-terminated, padded to 4)
  b  blob (int32 length + bytes + pad)
  T  True (no payload)
  F  False (no payload)
  N  Nil (no payload)
  d  float64
  h  int64

For fuzzing we also support a synthetic 'X' tag that encodes raw bytes as-is
(no padding) — useful for crafting truncated/garbage payloads.
"""

from __future__ import annotations

import struct
from typing import Iterable


def _pad4(data: bytes) -> bytes:
    rem = (4 - (len(data) % 4)) % 4
    return data + b"\x00" * rem


def encode_string(s: str) -> bytes:
    return _pad4(s.encode("utf-8") + b"\x00")


def encode_blob(b: bytes) -> bytes:
    return struct.pack(">i", len(b)) + _pad4(b)


def encode_arg(tag: str, value) -> bytes:
    if tag == "i":
        return struct.pack(">i", int(value))
    if tag == "f":
        return struct.pack(">f", float(value))
    if tag == "s":
        return encode_string(str(value))
    if tag == "b":
        return encode_blob(bytes(value))
    if tag in ("T", "F", "N", "I"):
        return b""
    if tag == "d":
        return struct.pack(">d", float(value))
    if tag == "h":
        return struct.pack(">q", int(value))
    if tag == "X":
        return bytes(value)
    raise ValueError(f"unknown tag {tag!r}")


def encode_message(address: str, args: Iterable[tuple[str, object]]) -> bytes:
    """Encode a message from an explicit list of (tag, value) pairs.

    `args` may contain any tags this module knows. The tag string is built
    from those exactly — we do NOT validate or coerce. That's what lets us
    create wrong-tag packets on purpose.
    """
    args = list(args)
    tags = "," + "".join(t for t, _ in args)
    body = b"".join(encode_arg(t, v) for t, v in args)
    return encode_string(address) + encode_string(tags) + body


def encode_typed(address: str, *values) -> bytes:
    """Convenience: infer tags from Python types (int -> i, float -> f,
    str -> s, bytes -> b, True/False/None -> T/F/N). For fuzzing use
    encode_message with explicit tags when you want to lie."""
    tagged = []
    for v in values:
        if v is True:
            tagged.append(("T", v))
        elif v is False:
            tagged.append(("F", v))
        elif v is None:
            tagged.append(("N", v))
        elif isinstance(v, bool):
            tagged.append(("T" if v else "F", v))
        elif isinstance(v, int):
            tagged.append(("i", v))
        elif isinstance(v, float):
            tagged.append(("f", v))
        elif isinstance(v, str):
            tagged.append(("s", v))
        elif isinstance(v, (bytes, bytearray)):
            tagged.append(("b", bytes(v)))
        else:
            raise TypeError(f"cannot infer OSC tag for {type(v).__name__}")
    return encode_message(address, tagged)
