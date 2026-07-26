#!/usr/bin/env python3
"""Path-rot lint for the Experiments/ build scripts.

The scratch harnesses under Experiments/ are built by hand-written scripts that
reference the tree by relative path (e.g. ../../spatcore/gpu/MetalFdnBackend.mm).
Nothing compiles them in CI, so when the tree moves they rot silently: the
GPU sources moved Source/DSP/gpu -> spatcore/gpu in the Phase 2 extraction and
three harnesses stayed broken for weeks before anyone tried to build them.

    python tools/validation/experiments_path_lint.py

This checks that every relative path a build script names actually EXISTS,
rather than grepping for one known-stale string -- so it also catches the NEXT
move, not just the last one. Paths are resolved against the script's own
directory, because every one of these scripts starts with cd "$(dirname "$0")".

Comment lines are skipped: only build-affecting paths are checked. Exits 0 when
Experiments/ does not exist, so it is safe to keep in CI unconditionally.
"""

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
EXPERIMENTS = REPO_ROOT / "Experiments"

# Build scripts only -- these are what break a build when they rot.
SCRIPT_GLOBS = ("*.sh", "*.bat", "*.ps1")

# PARENT-relative tokens only ("../"), i.e. references that reach OUT of the
# harness directory into the shared tree. Those are the ones a tree move breaks.
#
# Deliberately NOT "./": every script ends with `echo "built: ./backend_test"`,
# and ./backend_test is a gitignored build OUTPUT. Matching it made this lint
# pass on a developer box that had built the harnesses and fail on any clean
# checkout -- i.e. it would have failed CI while looking green locally.
PATH_RE = re.compile(r"(?<![\w/])\.\./[^\s\"'`;|)]+")

COMMENT_PREFIXES = ("#", "::", "rem ", "REM ")


def is_comment(line: str) -> bool:
    stripped = line.lstrip()
    return any(stripped.startswith(p) for p in COMMENT_PREFIXES)


def main() -> int:
    if not EXPERIMENTS.is_dir():
        print("experiments path lint OK (no Experiments/ directory)")
        return 0

    scripts = sorted(
        p for g in SCRIPT_GLOBS for p in EXPERIMENTS.glob(f"*/{g}")
    )
    if not scripts:
        print("experiments path lint OK (no build scripts found)")
        return 0

    missing = []
    checked = 0

    for script in scripts:
        for lineno, line in enumerate(
            script.read_text(encoding="utf-8", errors="replace").splitlines(), 1
        ):
            if is_comment(line):
                continue
            for token in PATH_RE.findall(line):
                token = token.rstrip(".,:\\")
                if not token:
                    continue
                checked += 1
                # Scripts cd to their own directory first, so resolve there.
                if not (script.parent / token).exists():
                    rel = script.relative_to(REPO_ROOT)
                    missing.append(f"  {rel}:{lineno}  ->  {token}")

    if missing:
        print("experiments path lint FAILED - build scripts reference paths "
              "that do not exist:")
        print("\n".join(missing))
        print("The tree moved and these were not updated. Fix the paths (or "
              "delete the dead harness).")
        return 1

    print(f"experiments path lint OK ({checked} paths in {len(scripts)} "
          f"build scripts all resolve)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
