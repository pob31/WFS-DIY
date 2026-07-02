#!/usr/bin/env python3
"""Dependency-rule lint for the spatcore extraction.

Rule (docs/architecture/core-boundary-proposal-audio.md §3): spatcore must
never include, reference, or name anything from the app. Dependencies point
app -> spatcore only, never back. This scans every source/header under
spatcore/ for forbidden #includes and fails the build/CI on any hit.

    python tools/validation/spatcore_dep_lint.py

Exits 0 when spatcore/ does not exist yet (pre-Phase-1) so it can sit in CI
from day one.

`JuceHeader.h` is also forbidden: it is Projucer-generated per app; spatcore
code includes the specific juce module headers it needs (e.g.
<juce_audio_basics/juce_audio_basics.h>) so it builds as a standalone library.
"""

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
SPATCORE = REPO_ROOT / "spatcore"

# Forbidden in any #include path inside spatcore/ (third_party/ is exempt).
FORBIDDEN = [
    "Parameters/",
    "Network/",
    "gui/",
    "Controllers/",
    "Sampler/",
    "GradientMap/",
    "Localization/",
    "MainComponent",
    "WFSValueTreeState",
    "WFSParameterIDs",
    "WFSParameterDefaults",
    "WFSConstraints",
    "WfsParameters",
    "JuceHeader.h",
]

INCLUDE_RE = re.compile(r'^\s*#\s*include\s+["<]([^">]+)[">]')
EXTS = {".h", ".hpp", ".cpp", ".cc", ".mm", ".cu"}


def main() -> int:
    if not SPATCORE.is_dir():
        print("spatcore/ does not exist yet - nothing to lint (OK)")
        return 0

    hits = []
    for f in sorted(SPATCORE.rglob("*")):
        if f.suffix not in EXTS or "third_party" in f.parts:
            continue
        for lineno, line in enumerate(f.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
            m = INCLUDE_RE.match(line)
            if m and any(bad in m.group(1) for bad in FORBIDDEN):
                rel = f.relative_to(REPO_ROOT)
                hits.append(f"  {rel}:{lineno}: #include \"{m.group(1)}\"")

    if hits:
        print("spatcore dependency lint FAILED - app includes inside the core:")
        print("\n".join(hits))
        print("Rule: dependencies point app -> spatcore only, never back "
              "(docs/architecture/core-boundary-proposal-audio.md §3).")
        return 1

    print("spatcore dependency lint OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
