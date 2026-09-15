"""Minimal ctypes wrapper for the teVirtualMIDI driver (the one loopMIDI
installs): create a MIDI port that other applications see as a MIDI *input*,
and play short messages into it. Windows only, stdlib only.

The driver ships with loopMIDI (Tobias Erichsen); teVirtualMIDI64.dll lands in
System32. The port exists only while the creating process holds it, which is
what lets a test unplug and replug it at will.
"""

from __future__ import annotations

import ctypes
import ctypes.wintypes as wt

TE_VM_FLAGS_PARSE_RX = 1
TE_VM_FLAGS_INSTANTIATE_BOTH = 12

try:
    _dll = ctypes.WinDLL("teVirtualMIDI64.dll")
except OSError as exc:  # driver not installed
    raise ImportError("teVirtualMIDI64.dll not found (install loopMIDI)") from exc

_CB = ctypes.WINFUNCTYPE(None, ctypes.c_void_p, ctypes.POINTER(ctypes.c_ubyte),
                         wt.DWORD, ctypes.c_void_p)

_dll.virtualMIDICreatePortEx2.restype = ctypes.c_void_p
_dll.virtualMIDICreatePortEx2.argtypes = [wt.LPCWSTR, _CB, ctypes.c_void_p,
                                          wt.DWORD, wt.DWORD]
_dll.virtualMIDISendData.restype = wt.BOOL
_dll.virtualMIDISendData.argtypes = [ctypes.c_void_p,
                                     ctypes.POINTER(ctypes.c_ubyte), wt.DWORD]
_dll.virtualMIDIClosePort.restype = None
_dll.virtualMIDIClosePort.argtypes = [ctypes.c_void_p]


@_CB
def _ignore_rx(port, data, length, instance):  # nothing is sent to us
    return None


class VirtualPort:
    """One virtual MIDI port. Data sent here arrives at every application
    that opened the port as an input."""

    def __init__(self, name: str):
        self.name = name
        self.handle = _dll.virtualMIDICreatePortEx2(
            name, _ignore_rx, None, 65535,
            TE_VM_FLAGS_PARSE_RX | TE_VM_FLAGS_INSTANTIATE_BOTH)
        if not self.handle:
            raise OSError(f"virtualMIDICreatePortEx2('{name}') failed, "
                          f"GetLastError={ctypes.GetLastError()}")

    def send(self, *msg: int) -> None:
        buf = (ctypes.c_ubyte * len(msg))(*msg)
        if not _dll.virtualMIDISendData(self.handle, buf, len(msg)):
            raise OSError(f"virtualMIDISendData failed, "
                          f"GetLastError={ctypes.GetLastError()}")

    def note_on(self, channel: int, note: int, velocity: int) -> None:
        self.send(0x90 | ((channel - 1) & 0x0F), note & 0x7F, velocity & 0x7F)

    def note_off(self, channel: int, note: int, velocity: int = 64) -> None:
        self.send(0x80 | ((channel - 1) & 0x0F), note & 0x7F, velocity & 0x7F)

    def close(self) -> None:
        if self.handle:
            _dll.virtualMIDIClosePort(self.handle)
            self.handle = None
