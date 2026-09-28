"""Session round-trip driver.

Copies the golden fixture project to a temp folder, launches WFS-DIY on it
(WFS_MCP_AI_ENABLED=1), triggers a full save via the tier-2 `session_save`
MCP tool (confirm round-trip), closes the app gracefully, then diffs every
section XML against the committed fixture after normalization (the volatile
`<!-- Created: ... -->` header line is stripped on both sides).

Zero diff proves load -> in-memory state -> save is lossless for the whole
session surface. The committed fixture IS the golden; there is no --update
here (regenerate the fixture with the bootstrap procedure instead).

Before closing, a second save runs with a file where the backups folder
should be: it must fail, name the file and the folder, and leave every
section file as it was (audit S2).

Usage:
  python session_roundtrip.py [--exe path\\to\\WFS-DIY.exe] [--keep-temp]

Exit codes: 0 pass, 1 mismatch, 2 usage, 3 app failed to start.
"""

from __future__ import annotations

import argparse
import os
import shutil
import sys
from pathlib import Path

import common


def check_save_refused_without_backup(app: common.App,
                                      project: Path) -> list[str]:
    """A save backs each file up before replacing it, and must stop when the
    backup cannot be made (audit S2): the pre-fix build ignored the failure
    and saved over every file anyway. A file named `backups` takes the
    folder's place; each section file gets a marker line that a save would
    wipe. Everything is put back before returning, so the round-trip diff
    after close is unaffected. Returns the failures."""
    failures: list[str] = []
    backups = project / "backups"
    parked = project / "backups.parked"
    originals = {name: (project / name).read_bytes()
                 for name in common.SECTION_FILES}
    marker = b"<!-- roundtrip: a save must not replace this file -->\r\n"
    backups.rename(parked)
    try:
        backups.write_text("a file where the backups folder should be")
        for name, data in originals.items():
            (project / name).write_bytes(data + marker)

        _, final = app.tool_confirmed("session_save", {})
        result = common.envelope_result(final)
        text = " ".join(c.get("text", "") for c in result.get("content", []))
        if not result.get("isError"):
            failures.append(f"a save with no backup folder reported success: {text[:200]}")
        elif "system.xml" not in text or "backups" not in text:
            # The app language may not be English: match the names only.
            failures.append(f"the refusal does not name the file and folder: {text[:200]}")
        for name, data in originals.items():
            if (project / name).read_bytes() != data + marker:
                failures.append(f"{name} was saved over although its backup failed")
    finally:
        if backups.is_file():
            backups.unlink()
        parked.rename(backups)
        for name, data in originals.items():
            (project / name).write_bytes(data)
    if not failures:
        print("[roundtrip] PASS a save whose backups fail leaves every file as it was")
    return failures


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--exe", default=None,
                   help="WFS-DIY.exe path (default: auto-probe Release then "
                        "Debug build dirs)")
    p.add_argument("--keep-temp", action="store_true",
                   help="Leave the temp project copy behind for inspection")
    args = p.parse_args()

    exe = common.find_exe(args.exe)
    if not common.FIXTURE_DIR.is_dir():
        print(f"[roundtrip] fixture missing: {common.FIXTURE_DIR}",
              file=sys.stderr)
        return common.EXIT_USAGE

    work_root = Path(os.environ.get("TEMP", ".")) / "wfs-control-replay" \
        / "session_roundtrip"
    project = common.copy_fixture_to_temp(work_root)
    wfs = common.fixture_wfs(project)

    common.kill_stale_instances()
    app = common.App(exe, wfs, ai_enabled=True)
    try:
        app.wait_for_mcp()
        # OSCQuery starting proves network.xml was actually ingested — the
        # project load is async and finishes after the MCP server is up.
        app.wait_for_oscquery()

        first, final = app.tool_confirmed("session_save", {})
        tier = common.envelope_result(first).get("tier_enforcement", {})
        if not tier.get("awaiting_confirmation"):
            print("[roundtrip] WARNING: session_save was not tier-2 gated "
                  f"(tier_enforcement={tier})", file=sys.stderr)
        payload = common.tool_payload(final)
        if not (isinstance(payload, dict) and payload.get("saved") is True):
            print(f"[roundtrip] session_save failed: {payload}",
                  file=sys.stderr)
            return common.EXIT_MISMATCH

        backup_failures = check_save_refused_without_backup(app, project)
    finally:
        graceful = app.close()
    if not graceful:
        print("[roundtrip] WARNING: close was not graceful", file=sys.stderr)

    failures = len(backup_failures)
    for f in backup_failures:
        print(f"[roundtrip] FAIL {f}", file=sys.stderr)
    for name in common.SECTION_FILES:
        fixture_text = common.normalize_xml_text(
            (common.FIXTURE_DIR / name).read_text(encoding="utf-8"))
        saved = project / name
        if not saved.is_file():
            print(f"[roundtrip] MISSING after save: {name}", file=sys.stderr)
            failures += 1
            continue
        actual_text = common.normalize_xml_text(
            saved.read_text(encoding="utf-8"))
        if fixture_text == actual_text:
            print(f"[roundtrip] PASS {name}")
        else:
            failures += 1
            print(f"[roundtrip] DIFF {name}:", file=sys.stderr)
            sys.stderr.write(common.unified_diff(
                fixture_text, actual_text,
                f"fixture/{name}", f"saved/{name}"))

    if not args.keep_temp:
        shutil.rmtree(work_root, ignore_errors=True)

    if failures:
        print(f"[roundtrip] FAIL: {failures} section(s) diverged",
              file=sys.stderr)
        return common.EXIT_MISMATCH
    print("[roundtrip] PASS: full-session round-trip is lossless")
    return common.EXIT_PASS


if __name__ == "__main__":
    raise SystemExit(main())
