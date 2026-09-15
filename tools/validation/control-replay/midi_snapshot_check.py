"""MIDI note -> input snapshot recall, driven through a real MIDI port.

Creates a Windows MIDI input port with the teVirtualMIDI driver (installed by
loopMIDI), points WFS-DIY at it through WFS-DIY.settings, stores snapshots over
OSC, binds them to notes by writing the <InputSnapshot> root attributes, and
plays notes into the port. Recall is observed through OSCQuery: input 1's
attenuation carries a distinct value per snapshot.

Covers the documented contract (velocity > 64 fires, note-offs and softer
notes do not, 250 ms same-key lockout, the later of two cues wins, bindings
follow the files) and the 1.0.0beta51 fixes: a cue pressed while the previous
recall is loading, a fast unplug/replug, edits made on disk behind the app's
back, a half-written binding, a failed recall being logged, a collision
present at launch, and the name fallback leaving the saved identifier alone.

WFS-DIY.settings is backed up and restored around the run (the test injects the
MIDI input selection and, for the last case, lastProjectFolder).

Assertion-based (no golden). Exit codes per common.py; 2 also means the
teVirtualMIDI driver is not installed.

Usage: python midi_snapshot_check.py [--exe PATH] [--only NAME,...] [--keep-temp]
"""

from __future__ import annotations

import argparse
import glob
import os
import re
import shutil
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common  # noqa: E402

try:
    import vmidi  # noqa: E402
except ImportError as exc:
    print(f"[midi] SKIP: {exc}", file=sys.stderr)
    raise SystemExit(common.EXIT_USAGE)

PORT_NAME = "WFS-MIDI-Test"
SETTINGS = Path(os.environ["APPDATA"]) / "WFS-DIY" / "WFS-DIY.settings"
LOG_DIR = Path(os.environ["APPDATA"]) / "WFS-DIY" / "logs"
ATT = "/wfs/input/attenuation"
ATT_Q = "/wfs/input/1/attenuation"

# snapshot name -> (attenuation marker, (channel, note))
SNAPS = {
    "snapA": (-3.0, (1, 60)),
    "snapB": (-9.0, (1, 62)),
    "snapC": (-15.0, (2, 64)),
    "snapE": (-21.0, (1, 65)),
    "snapF": (-27.0, (1, 67)),
}
NEUTRAL = 0.0

FAILURES: list[str] = []


def check(cond: bool, label: str, detail: str = "") -> bool:
    if cond:
        print(f"[midi] PASS  {label}  {detail}")
    else:
        FAILURES.append(label)
        print(f"[midi] FAIL  {label}  {detail}", file=sys.stderr)
    return cond


def att() -> float:
    v = common.oscquery_get(ATT_Q)
    if isinstance(v, list):
        v = v[0]
    return float(v)


def hold(seconds: float) -> None:
    time.sleep(seconds)


class Rig:
    def __init__(self, app: common.App, port: vmidi.VirtualPort,
                 project: Path):
        self.app = app
        self.port = port
        self.project = project
        self.osc = common.OSCSender(delay=0.0)
        self.snapdir = project / "snapshots" / "inputs"

    def set_att(self, v: float, settle: float = 0.25) -> None:
        self.osc.send(ATT, [("i", 1), ("f", float(v))])
        deadline = time.monotonic() + 2.0
        while time.monotonic() < deadline:
            if abs(att() - v) < 1e-3:
                break
            time.sleep(0.02)
        time.sleep(settle)

    def store(self, name: str) -> None:
        self.osc.send("/wfs/input/snapshot/store", [("s", name)])
        f = self.snapdir / f"{name}.xml"
        deadline = time.monotonic() + 5.0
        while time.monotonic() < deadline and not f.is_file():
            time.sleep(0.05)
        time.sleep(0.3)

    def load_osc(self, name: str) -> None:
        self.osc.send("/wfs/input/snapshot/load", [("s", name)])

    def bind_file(self, name: str, ch: int | None, note: int | None) -> None:
        """Write (or remove) the binding on the file's root element."""
        f = self.snapdir / f"{name}.xml"
        text = f.read_text(encoding="utf-8")
        text = re.sub(r'\s+midiChannel="[^"]*"', "", text)
        text = re.sub(r'\s+midiNote="[^"]*"', "", text)
        if ch is not None:
            text = re.sub(r"<InputSnapshot\b",
                          f'<InputSnapshot midiChannel="{ch}" midiNote="{note}"',
                          text, count=1)
        f.write_text(text, encoding="utf-8")

    def root_attrs(self, name: str) -> str:
        text = (self.snapdir / f"{name}.xml").read_text(encoding="utf-8")
        m = re.search(r"<InputSnapshot\b[^>]*>", text)
        return m.group(0) if m else "<none>"

    def refresh_index(self) -> None:
        """An OSC store of a scratch snapshot ends in refreshMidiSnapshotBindings."""
        self.store("zz_refresh")
        time.sleep(0.3)

    def wait_value(self, target: float, timeout: float = 1.5) -> float | None:
        """Seconds until input 1 attenuation reads `target`, or None."""
        t0 = time.monotonic()
        while time.monotonic() - t0 < timeout:
            if abs(att() - target) < 1e-3:
                return time.monotonic() - t0
            time.sleep(0.01)
        return None

    def stays(self, value: float, duration: float = 0.7) -> bool:
        t0 = time.monotonic()
        while time.monotonic() - t0 < duration:
            if abs(att() - value) > 1e-3:
                return False
            time.sleep(0.02)
        return True


# ---------------------------------------------------------------------------
# Test cases
# ---------------------------------------------------------------------------

def t_basic(r: Rig) -> None:
    for name, (val, (ch, note)) in SNAPS.items():
        r.set_att(NEUTRAL)
        hold(0.3)  # clear of the 250 ms same-key lockout
        r.port.note_on(ch, note, 100)
        dt = r.wait_value(val)
        check(dt is not None, f"basic: ch{ch} note {note} recalls {name}",
              f"latency={dt and round(dt * 1000)} ms, value={att()}")
        r.port.note_off(ch, note)


def t_velocity(r: Rig) -> None:
    val, (ch, note) = SNAPS["snapA"]
    r.set_att(NEUTRAL); hold(0.3)
    r.port.note_on(ch, note, 64)
    check(r.stays(NEUTRAL), "velocity 64 is ignored")
    r.port.note_on(ch, note, 0)
    check(r.stays(NEUTRAL), "velocity-0 note-on is ignored")
    r.port.note_off(ch, note, 127)
    check(r.stays(NEUTRAL), "note-off (vel 127) is ignored")
    r.port.note_on(ch, note, 65)
    check(r.wait_value(val) is not None, "velocity 65 fires")


def t_unbound(r: Rig) -> None:
    r.set_att(NEUTRAL); hold(0.3)
    r.port.note_on(1, 61, 100)
    check(r.stays(NEUTRAL), "unbound note (ch1 61) does nothing")
    r.port.note_on(3, 60, 100)
    check(r.stays(NEUTRAL), "bound note on the wrong channel (ch3 60) does nothing")
    r.port.send(0xB0, 7, 100)                          # CC
    r.port.send(0xC0, 5)                               # program change
    r.port.send(0xE0, 0, 64)                           # pitch bend
    r.port.send(0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7)    # sysex identity request
    check(r.stays(NEUTRAL), "CC / program change / pitch bend / sysex do nothing")
    check(r.app.alive(), "app survives non-note traffic")


def t_lockout(r: Rig) -> None:
    """A recall blocks the message thread, so an OSC reset sent right behind
    the first hit lands only once that recall is done. The second hit of the
    same key inside 250 ms must not recall again after the reset."""
    val, (ch, note) = SNAPS["snapA"]
    r.set_att(NEUTRAL); hold(0.3)
    t0 = time.monotonic()
    r.port.note_on(ch, note, 100)
    time.sleep(0.02)
    r.osc.send(ATT, [("i", 1), ("f", NEUTRAL)])   # queued behind the recall
    time.sleep(max(0.0, 0.15 - (time.monotonic() - t0)))
    since = time.monotonic() - t0
    r.port.note_on(ch, note, 100)                 # same key, inside 250 ms
    hold(1.2)
    final = att()
    check(since < 0.25 and abs(final - NEUTRAL) < 1e-3,
          "lockout: same key again inside 250 ms does not recall again",
          f"second hit at +{round(since * 1000)} ms, final={final}")
    hold(0.3)
    r.port.note_on(ch, note, 100)
    check(r.wait_value(val) is not None,
          "lockout: same key after 250 ms fires again")


def t_two_notes(r: Rig) -> None:
    """Two DIFFERENT bound notes: the later must end up applied."""
    va, (cha, na) = SNAPS["snapA"]
    vb, (chb, nb) = SNAPS["snapB"]
    for gap_ms in (0, 10, 20, 40, 80, 150, 300, 600):
        r.set_att(NEUTRAL); hold(0.4)
        r.port.note_on(cha, na, 100)
        if gap_ms:
            time.sleep(gap_ms / 1000.0)
        r.port.note_on(chb, nb, 100)
        hold(1.0)
        final = att()
        check(abs(final - vb) < 1e-3,
              f"two notes {gap_ms} ms apart: the later (snapB) is applied",
              f"final={final} (snapA={va}, snapB={vb})")


def t_note_during_recall(r: Rig) -> None:
    """A cue pressed while the previous recall is still loading. The value
    flips early in a recall (the tree write), before handleConfigReloaded,
    so notes sent the moment A's value shows land inside A's recall."""
    va, (cha, na) = SNAPS["snapA"]
    vb, (chb, nb) = SNAPS["snapB"]
    vc, (chc, nc) = SNAPS["snapC"]
    for _ in range(3):
        r.set_att(NEUTRAL); hold(0.4)
        r.port.note_on(cha, na, 100)
        r.wait_value(va)
        r.port.note_on(chb, nb, 100)
        hold(1.2)
        final = att()
        check(abs(final - vb) < 1e-3,
              "a cue pressed during the previous recall is applied after it",
              f"final={final} (A={va}, B={vb})")
    r.set_att(NEUTRAL); hold(0.4)
    r.port.note_on(cha, na, 100)
    r.wait_value(va)
    r.port.note_on(chb, nb, 100)
    time.sleep(0.003)
    r.port.note_on(chc, nc, 100)
    hold(1.2)
    final = att()
    check(abs(final - vc) < 1e-3,
          "two cues pressed during a recall: the latest wins",
          f"final={final} (B={vb}, C={vc})")


def t_aba(r: Rig) -> None:
    va, (cha, na) = SNAPS["snapA"]
    vb, (chb, nb) = SNAPS["snapB"]
    r.set_att(NEUTRAL); hold(0.4)
    r.port.note_on(cha, na, 100); hold(0.1)
    r.port.note_on(chb, nb, 100); hold(0.1)
    r.port.note_on(cha, na, 100); hold(1.0)
    check(abs(att() - va) < 1e-3, "A, B, A 100 ms apart ends on A",
          f"final={att()}")


def t_update_keeps_binding(r: Rig) -> None:
    val, (ch, note) = SNAPS["snapA"]
    r.set_att(-4.5)
    r.store("snapA")                 # OSC store over the bound snapshot
    attrs = r.root_attrs("snapA")
    check(f'midiChannel="{ch}"' in attrs and f'midiNote="{note}"' in attrs,
          "OSC store over a bound snapshot keeps the binding in the file", attrs)
    r.set_att(NEUTRAL); hold(0.3)
    r.port.note_on(ch, note, 100)
    check(r.wait_value(-4.5) is not None,
          "after OSC update the note recalls the updated values", f"value={att()}")
    r.set_att(val); r.store("snapA")   # put the original marker back


def t_osc_load_then_midi(r: Rig) -> None:
    """A recall from OSC rebuilds the index; MIDI must keep working."""
    vb, _ = SNAPS["snapB"]
    va, (cha, na) = SNAPS["snapA"]
    r.set_att(NEUTRAL); hold(0.3)
    r.load_osc("snapB")
    check(r.wait_value(vb) is not None, "OSC load recalls snapB")
    hold(0.3)
    r.port.note_on(cha, na, 100)
    check(r.wait_value(va) is not None, "MIDI still recalls after an OSC load")


def t_rebind_live(r: Rig) -> None:
    """Rebinding snapA to another note: the old note stops, the new fires."""
    va, (cha, na) = SNAPS["snapA"]
    r.bind_file("snapA", 4, 70)
    r.refresh_index()
    r.set_att(NEUTRAL); hold(0.3)
    r.port.note_on(cha, na, 100)
    check(r.stays(NEUTRAL), "rebind: the old note no longer fires")
    r.port.note_on(4, 70, 100)
    check(r.wait_value(va) is not None, "rebind: the new note fires")
    r.bind_file("snapA", cha, na)
    r.refresh_index()


def t_duplicate(r: Rig, log_path: Path | None) -> None:
    va, (cha, na) = SNAPS["snapA"]
    r.set_att(-33.0)
    r.store("snapZdup")
    r.bind_file("snapZdup", cha, na)           # same key as snapA, sorts later
    r.refresh_index()
    r.set_att(NEUTRAL); hold(0.3)
    r.port.note_on(cha, na, 100)
    check(r.wait_value(va) is not None,
          "on-disk duplicate: first in snapshot-name order (snapA) wins",
          f"value={att()}")
    if log_path is not None:
        lines = conflict_lines(log_path, "snapZdup")
        check(any("snapA" in l.replace("snapZdup", "") for l in lines),
              "on-disk duplicate is logged naming both snapshots", f"lines={len(lines)}")
    r.bind_file("snapZdup", None, None)
    r.refresh_index()


def t_hotplug(r: Rig) -> None:
    va, (cha, na) = SNAPS["snapA"]
    r.port.close()
    hold(2.0)
    r.port = vmidi.VirtualPort(PORT_NAME)
    hold(2.0)
    r.set_att(NEUTRAL); hold(0.3)
    r.port.note_on(cha, na, 100)
    check(r.wait_value(va, 2.0) is not None,
          "hot-plug: port closed and re-created, note recalls again")


def t_fast_replug(r: Rig) -> None:
    """The port vanishes and comes back inside JUCE's 500 ms debounce, so the
    device list reads the same before and after."""
    va, (cha, na) = SNAPS["snapA"]
    for gap in (0.05, 0.2):
        r.port.close()
        time.sleep(gap)
        r.port = vmidi.VirtualPort(PORT_NAME)
        hold(2.5)
        r.set_att(NEUTRAL); hold(0.3)
        r.port.note_on(cha, na, 100)
        check(r.wait_value(va, 2.0) is not None,
              f"fast replug ({int(gap * 1000)} ms): note recalls again")


def t_external_edit(r: Rig) -> None:
    """Changes made on disk behind the app's back reach the index without any
    in-app store or recall (the 1 s folder poll)."""
    r.set_att(-36.0)
    r.store("snapG")                       # in-app store: index refreshed now
    r.bind_file("snapG", 1, 69)            # external edit: no refresh call
    hold(1.6)
    r.set_att(NEUTRAL); hold(0.3)
    r.port.note_on(1, 69, 100)
    check(r.wait_value(-36.0) is not None,
          "binding added on disk is armed without an in-app action")

    # Explorer-style rename of a bound snapshot
    vb, (chb, nb) = SNAPS["snapB"]
    (r.snapdir / "snapB.xml").rename(r.snapdir / "snapB-renamed.xml")
    hold(1.6)
    r.set_att(NEUTRAL); hold(0.3)
    r.port.note_on(chb, nb, 100)
    check(r.wait_value(vb) is not None,
          "snapshot renamed on disk still recalls from its note")
    (r.snapdir / "snapB-renamed.xml").rename(r.snapdir / "snapB.xml")
    hold(1.6)

    # A half-written binding (midiChannel only) is unbound, not note 0
    f = r.snapdir / "snapG.xml"
    text = f.read_text(encoding="utf-8")
    f.write_text(re.sub(r'\s+midiNote="[^"]*"', "", text), encoding="utf-8")
    hold(1.6)
    r.set_att(NEUTRAL); hold(0.3)
    r.port.note_on(1, 0, 100)
    check(r.stays(NEUTRAL), "midiChannel without midiNote does not arm note 0")
    r.port.note_on(1, 69, 100)
    check(r.stays(NEUTRAL), "... and the old note is no longer armed either")
    r.bind_file("snapG", None, None)
    hold(1.6)


def t_failed_recall(r: Rig, log_path: Path | None) -> None:
    """A bound snapshot whose body is broken: the MIDI recall fails and says so."""
    r.set_att(-39.0)
    r.store("snapH")
    r.bind_file("snapH", 1, 71)
    f = r.snapdir / "snapH.xml"
    text = f.read_text(encoding="utf-8")
    cut = text.find("<Inputs")
    f.write_text(text[:cut + 40], encoding="utf-8")   # root intact, body truncated
    hold(1.6)
    r.set_att(NEUTRAL); hold(0.3)
    r.port.note_on(1, 71, 100)
    check(r.stays(NEUTRAL), "broken snapshot applies nothing")
    hold(0.3)
    if log_path is not None:
        txt = log_path.read_text(encoding="utf-8", errors="replace")
        check("Snapshot recall of 'snapH' (MIDI) failed" in txt,
              "the failed MIDI recall is logged")
    f.unlink()
    hold(1.6)


TESTS = {
    "basic": t_basic,
    "velocity": t_velocity,
    "unbound": t_unbound,
    "lockout": t_lockout,
    "two_notes": t_two_notes,
    "note_during_recall": t_note_during_recall,
    "aba": t_aba,
    "update_keeps_binding": t_update_keeps_binding,
    "osc_load_then_midi": t_osc_load_then_midi,
    "rebind_live": t_rebind_live,
    "duplicate": t_duplicate,          # takes the log path
    "hotplug": t_hotplug,
    "fast_replug": t_fast_replug,
    "external_edit": t_external_edit,
    "failed_recall": t_failed_recall,  # takes the log path
}
NEEDS_LOG = {"duplicate", "failed_recall"}


# ---------------------------------------------------------------------------
# Plumbing
# ---------------------------------------------------------------------------

def newest_log(since: float) -> Path | None:
    logs = [Path(p) for p in glob.glob(str(LOG_DIR / "WFS-DIY_*.log"))]
    logs = [p for p in logs if p.stat().st_mtime >= since - 1]
    return max(logs, key=lambda p: p.stat().st_mtime) if logs else None


def conflict_lines(log_path: Path | None, loser: str) -> list[str]:
    """Warning lines naming `loser`. Matched on the snapshot name, not on the
    message text, which follows the app's language setting."""
    if log_path is None:
        return []
    text = log_path.read_text(encoding="utf-8", errors="replace")
    return [l for l in text.splitlines() if loser in l and "WARN" in l]


def inject_settings(identifier: str, name: str) -> None:
    text = SETTINGS.read_text(encoding="utf-8")
    text = re.sub(r'\s*<VALUE name="midiSnapshotInput(Id|Name)"[^>]*/>', "", text)
    add = (f'\n  <VALUE name="midiSnapshotInputId" val="{identifier}"/>'
           f'\n  <VALUE name="midiSnapshotInputName" val="{name}"/>')
    SETTINGS.write_text(text.replace("<PROPERTIES>", "<PROPERTIES>" + add, 1),
                        encoding="utf-8")


def launch_duplicate(exe: Path, project: Path, port: vmidi.VirtualPort) -> None:
    """Launch with a collision already on disk and the project restored from
    lastProjectFolder (no .wfs argument, so no config reload runs): the
    warning must still be logged at startup, and only once however many
    recalls follow."""
    snapdir = project / "snapshots" / "inputs"
    dup = snapdir / "snapA - Copy.xml"
    shutil.copy2(snapdir / "snapA.xml", dup)   # an Explorer copy carries the note

    text = SETTINGS.read_text(encoding="utf-8")
    text = re.sub(r'<VALUE name="lastProjectFolder"[^>]*/>',
                  lambda _m: f'<VALUE name="lastProjectFolder" val="{project}"/>', text)
    SETTINGS.write_text(text, encoding="utf-8")

    started = time.time()
    app = common.App(exe, None, ai_enabled=False)
    try:
        loser = "snapA - Copy"
        log_path, lines = None, []
        deadline = time.monotonic() + 20.0
        while time.monotonic() < deadline and app.alive():
            log_path = newest_log(started)
            lines = conflict_lines(log_path, loser)
            if lines:
                break
            time.sleep(0.5)
        check(len(lines) == 1, "collision on disk at launch is logged",
              f"log={log_path.name if log_path else None} lines={lines}")
        _, (ch, note) = SNAPS["snapA"]
        for _ in range(3):
            port.note_on(ch, note, 100)
            hold(0.8)
        count = len(conflict_lines(log_path, loser))
        check(count == 1, "the collision is reported once, not on every recall",
              f"count={count}")
    finally:
        app.close()
        dup.unlink(missing_ok=True)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default=None)
    ap.add_argument("--only", default="", help="comma-separated test names "
                    f"({', '.join(list(TESTS) + ['launch_duplicate'])})")
    ap.add_argument("--keep-temp", action="store_true")
    args = ap.parse_args()

    exe = common.find_exe(args.exe)
    only = [s for s in args.only.split(",") if s]

    work_root = Path(os.environ.get("TEMP", ".")) / "wfs-control-replay" / "midi_snapshot"
    project = common.copy_fixture_to_temp(work_root)

    common.kill_stale_instances()
    backup = work_root.parent / "WFS-DIY.settings.midi-backup"
    shutil.copy2(SETTINGS, backup)
    port = vmidi.VirtualPort(PORT_NAME)
    started = time.time()
    app = None
    try:
        # A stale identifier: the port is found by its name, and that fallback
        # must not overwrite the saved identifier.
        inject_settings("stale-identifier-from-another-boot", PORT_NAME)
        app = common.App(exe, common.fixture_wfs(project), ai_enabled=False)
        app.wait_for_mcp()
        app.wait_for_oscquery()
        time.sleep(1.0)

        r = Rig(app, port, project)
        for name, (val, bind) in SNAPS.items():
            r.set_att(val, settle=0.1)
            r.store(name)
            r.bind_file(name, *bind)
        r.refresh_index()

        for name, fn in TESTS.items():
            if only and name not in only:
                continue
            print(f"[midi] --- {name}")
            if name in NEEDS_LOG:
                fn(r, newest_log(started))
            else:
                fn(r)
        port = r.port

        app.close()
        app = None
        after = SETTINGS.read_text(encoding="utf-8")
        check('val="stale-identifier-from-another-boot"' in after,
              "the name fallback leaves the saved identifier alone",
              str(re.findall(r'<VALUE name="midiSnapshotInputId"[^>]*/>', after)))

        if not only or "launch_duplicate" in only:
            print("[midi] --- launch_duplicate")
            launch_duplicate(exe, project, port)
    finally:
        if app is not None:
            app.close()
        try:
            port.close()
        except OSError:
            pass
        shutil.copy2(backup, SETTINGS)
        print("[midi] WFS-DIY.settings restored")
        if not args.keep_temp:
            shutil.rmtree(work_root, ignore_errors=True)

    if FAILURES:
        print(f"[midi] {len(FAILURES)} failure(s): {FAILURES}", file=sys.stderr)
        return common.EXIT_MISMATCH
    print("[midi] ALL PASS")
    return common.EXIT_PASS


if __name__ == "__main__":
    raise SystemExit(main())
