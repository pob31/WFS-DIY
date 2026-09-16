#!/usr/bin/env python3
"""Minimal WAV reader/writer for the effects A/B harness.

Deliberately dependency-light (numpy only): the harness has to read whatever
Max/MSP's `sfrecord~` or Buffer export produces, which in practice is one of
16-bit PCM, 24-bit PCM, 32-bit PCM, 32-bit IEEE float or 64-bit IEEE float, in
either a plain `fmt ` chunk or a WAVE_FORMAT_EXTENSIBLE one. `wave` from the
standard library refuses the float cases, and scipy is not installed on the dev
box, so this module does the RIFF parsing itself.

Everything comes back as float64 in a (numSamples, numChannels) array, scaled
so that full scale is +-1.0. Integer formats are divided by 2**(bits-1), which
is the convention Max, JUCE and every DAW agree on for reading; the tiny
asymmetry at exactly -1.0 is irrelevant at the dB levels this harness reports.
"""

import struct
import numpy as np

WAVE_FORMAT_PCM = 0x0001
WAVE_FORMAT_IEEE_FLOAT = 0x0003
WAVE_FORMAT_EXTENSIBLE = 0xFFFE


class WavError(Exception):
    pass


def _chunks(data):
    """Yield (id, payload) for every chunk in a RIFF body."""
    pos = 0
    end = len(data)
    while pos + 8 <= end:
        cid = data[pos:pos + 4]
        (size,) = struct.unpack_from('<I', data, pos + 4)
        body = data[pos + 8:pos + 8 + size]
        yield cid, body
        pos += 8 + size + (size & 1)      # chunks are word aligned


def read(path):
    """Read a WAV file.

    Returns (samples, sampleRate) where samples is float64
    (numSamples, numChannels).
    """
    with open(path, 'rb') as f:
        raw = f.read()

    if len(raw) < 12 or raw[0:4] != b'RIFF' or raw[8:12] != b'WAVE':
        raise WavError('%s: not a RIFF/WAVE file' % path)

    fmt = None
    data = None
    for cid, body in _chunks(raw[12:]):
        if cid == b'fmt ' and fmt is None:
            fmt = body
        elif cid == b'data' and data is None:
            data = body

    if fmt is None or data is None:
        raise WavError('%s: missing fmt or data chunk' % path)
    if len(fmt) < 16:
        raise WavError('%s: short fmt chunk (%d bytes)' % (path, len(fmt)))

    tag, channels, rate, _bytes_s, _align, bits = struct.unpack_from('<HHIIHH', fmt, 0)

    if tag == WAVE_FORMAT_EXTENSIBLE:
        if len(fmt) < 40:
            raise WavError('%s: truncated WAVE_FORMAT_EXTENSIBLE fmt chunk' % path)
        # cbSize(2) validBits(2) channelMask(4) then a 16-byte GUID whose first
        # two bytes are the real format tag.
        (tag,) = struct.unpack_from('<H', fmt, 24)

    if channels < 1:
        raise WavError('%s: %d channels' % (path, channels))

    if tag == WAVE_FORMAT_IEEE_FLOAT:
        if bits == 32:
            flat = np.frombuffer(data, dtype='<f4').astype(np.float64)
        elif bits == 64:
            flat = np.frombuffer(data, dtype='<f8').astype(np.float64)
        else:
            raise WavError('%s: %d-bit float is not a thing' % (path, bits))
    elif tag == WAVE_FORMAT_PCM:
        if bits == 16:
            flat = np.frombuffer(data, dtype='<i2').astype(np.float64) / 32768.0
        elif bits == 24:
            usable = (len(data) // 3) * 3
            b = np.frombuffer(data[:usable], dtype=np.uint8).reshape(-1, 3).astype(np.int32)
            v = b[:, 0] | (b[:, 1] << 8) | (b[:, 2] << 16)
            v = np.where(v >= (1 << 23), v - (1 << 24), v)
            flat = v.astype(np.float64) / 8388608.0
        elif bits == 32:
            flat = np.frombuffer(data, dtype='<i4').astype(np.float64) / 2147483648.0
        elif bits == 8:
            flat = (np.frombuffer(data, dtype=np.uint8).astype(np.float64) - 128.0) / 128.0
        else:
            raise WavError('%s: unsupported PCM bit depth %d' % (path, bits))
    else:
        raise WavError('%s: unsupported format tag 0x%04X' % (path, tag))

    usable = (len(flat) // channels) * channels
    return flat[:usable].reshape(-1, channels), int(rate)


def write_float32(path, samples, sample_rate):
    """Write a 32-bit IEEE-float WAV.

    `samples` may be 1-D (mono) or (numSamples, numChannels). Float is the
    format the harness generates in, so nothing the tool produces itself is
    ever quantised: a residual reported at -120 dB is a real -120 dB.
    """
    a = np.asarray(samples, dtype=np.float32)
    if a.ndim == 1:
        a = a.reshape(-1, 1)
    n, ch = a.shape

    payload = a.reshape(-1).astype('<f4').tobytes()
    if len(payload) & 1:
        payload += b'\x00'

    block_align = ch * 4
    fmt = struct.pack('<HHIIHHH',
                      WAVE_FORMAT_IEEE_FLOAT, ch, int(sample_rate),
                      int(sample_rate) * block_align, block_align, 32, 0)

    body = (b'WAVE'
            + b'fmt ' + struct.pack('<I', len(fmt)) + fmt
            + b'fact' + struct.pack('<II', 4, n)
            + b'data' + struct.pack('<I', len(payload)) + payload)

    with open(path, 'wb') as f:
        f.write(b'RIFF' + struct.pack('<I', len(body)) + body)


def mono(samples):
    """Collapse to one channel: channel 0, which is what a gen~ patch renders."""
    a = np.asarray(samples, dtype=np.float64)
    return a[:, 0] if a.ndim == 2 else a
