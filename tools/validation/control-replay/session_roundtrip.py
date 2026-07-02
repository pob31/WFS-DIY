"""Session round-trip driver.

Copies the golden fixture project to a temp folder, launches WFS-DIY on it
(WFS_MCP_AI_ENABLED=1), triggers a full save via the tier-2 `session_save`
MCP tool (confirm round-trip), closes the app gracefully, then diffs every
section XML against the committed fixture after normalization (the volatile
`<!-- Created: ... -->` header line is stripped on both sides).

Zero diff proves load -> in-memory state -> save is lossless for the whole
session surface. The committed fixture IS the golden; there is no --update
here (regenerate the fixture with the bootstrap procedure instead).

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
    finally:
        graceful = app.close()
    if not graceful:
        print("[roundtrip] WARNING: close was not graceful", file=sys.stderr)

    failures = 0
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
