# WFS-DIY OSC fuzzer

External Python OSC fuzzer. Talks to a running WFS-DIY instance over UDP
and looks for crashes, log warnings, and NaN/Inf leaking into the
ValueTree.

No app code changes required. Stdlib only — no `pip install` step.

## Layout

- `osc_fuzz.py` — main entry point.
- `osc_codec.py` — minimal hand-rolled OSC encoder (so we can emit
  intentionally malformed packets).
- `corpus.py` — Layer 1: hand-crafted nasty packets (NaN/Inf, type
  confusion, truncated, oversized, address typos, raw garbage).
- `addresses.py` — Layer 2: ~30 representative addresses with edge-case
  value pools per parameter kind.
- `findings/<timestamp>/summary.csv` — per-run findings, one row per
  signal.

## Prereqs

1. Build & launch WFS-DIY normally.
2. Open the System Config tab and note:
   - **UDP receive port** (the OSC listener) — required.
   - **OSCQuery HTTP port** (if enabled) — optional, used for read-back.
3. Find the WFS-DIY process ID (Task Manager / `Get-Process`) — optional,
   used for liveness checks.
4. The WFS-DIY log directory is **fixed** by the app:
   - **Windows:** `%APPDATA%/WFS-DIY/logs`
     i.e. `C:/Users/<you>/AppData/Roaming/WFS-DIY/logs`
   - **macOS:** `~/Library/Application Support/WFS-DIY/logs`
   - **Linux:** `~/.config/WFS-DIY/logs`
   Pass that path to `--log`. Optional, but enables log-tail scanning.
   The fuzzer warns if the path is wrong or has no `WFS-DIY_*.log` files.

If you skip any of (2)-(4), the fuzzer runs anyway and notes the gap in
the summary.

## Quickstart

Smoke test (sends one valid packet, verifies the harness):

    python osc_fuzz.py --udp 9000 --layer 1 \
        --pid 12345 --query 8080 \
        --log "C:/Users/you/AppData/Roaming/WFS-DIY/logs"

Layer 1 — hand-crafted nasty packets (~80 cases, runs in ~5s):

    python osc_fuzz.py --udp 9000 --layer 1 --pid <pid> --query <http>

Layer 2 — per-address structured fuzz (~600 packets, runs in ~30s):

    python osc_fuzz.py --udp 9000 --layer 2 --pid <pid> --query <http>

Layer 3 — random walk for 60 seconds:

    python osc_fuzz.py --udp 9000 --layer 3 --duration 60 --pid <pid>

All three layers in sequence:

    python osc_fuzz.py --udp 9000 --layer all --pid <pid> --query <http>

TCP transport (length-prefixed framing per OSC 1.0). Same corpus, same
layers; useful for testing the TCP receiver path which has its own
parsing entry. Default WFS-DIY TCP port is 8001:

    python osc_fuzz.py --tcp 8001 --layer 1 --pid <pid>

`--udp` and `--tcp` are mutually exclusive — pick one per run. The
fuzzer aborts immediately if it can't establish the initial TCP
connection, so make sure WFS-DIY is running and the TCP listener is on.

## What counts as a finding

Each finding is one row in `summary.csv` with a `signal` column:

- `crash` — the app PID is no longer running. Hard stop.
- `log` — a new `[WARNING]` / `[ERROR]` / `[CRITICAL]` line appeared
  in the active log file. The line itself is in the `detail` column.
- `readback` — OSCQuery returned a non-finite value (NaN / Inf) for a
  parameter the fuzzer just wrote. The path and value are in `detail`.
- `send-failed` — the codec couldn't encode a case. Almost always a bug
  in the corpus, not the app.

Exit code: `0` no findings, `1` findings recorded, `2` app died.

## Adding cases

- New Layer 1 case → append a `Case(...)` to `corpus.CASES` in
  `corpus.py`. Use the explicit `(tag, value)` form so wrong-tag packets
  are easy to express.
- New Layer 2 address → append an `AddrEntry(...)` to
  `addresses.ENTRIES`. Match the `shape` to the OSC argument layout the
  handler expects.
- Edge values per kind → edit `floats_for_kind()` / `ints_for_kind()` in
  `addresses.py`.

## Limitations

- File-format fuzzing (XML snapshots / lang JSON / IR audio) is **not**
  covered. Different harness.
- HID controllers (Stream Deck+, Xencelabs, SpaceMouse, Lightpad) are
  **not** covered — would need userland HID injection.
- The read-back signal depends on OSCQuery being enabled and reachable.
  When it isn't, NaN/Inf in the tree won't be detected here; you'd see
  the consequences only via crash or log noise.
- Layer 3 does detection every 50 packets to keep throughput up. A
  crash-on-packet-N where N isn't a multiple of 50 still gets caught,
  but the recorded "last batch" is approximate.
