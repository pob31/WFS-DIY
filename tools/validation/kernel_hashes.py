#!/usr/bin/env python3
"""Kernel-source hash gate for the spatcore extraction.

The GPU kernel sources are string literals in Source/DSP/gpu/*Kernels.h and are
byte-identical contracts: the CUDA-C headers are compiled at runtime by BOTH
NVRTC (CUDA) and hipRTC (HIP), and bit-exact GPU regression baselines are only
meaningful while the kernel text is unchanged. During the extraction these
files must move VERBATIM — never reformatted, re-namespaced, or re-encoded.

This tool records/checks SHA-256 hashes of every *Kernels.h:

    python tools/validation/kernel_hashes.py            # check against manifest
    python tools/validation/kernel_hashes.py --update   # regenerate manifest

Check mode exits non-zero on any mismatch/addition/removal, printing what
changed. Intentional kernel edits are committed together with an --update of
the manifest (and invalidate GPU output baselines — rerun the smoke tests).

The manifest keys are paths relative to the repo root so it survives the
planned move of the gpu tree into spatcore/ with a one-line GPU_DIR change.
"""

import argparse
import hashlib
import json
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
GPU_DIR = REPO_ROOT / "Source" / "DSP" / "gpu"
MANIFEST = Path(__file__).resolve().parent / "kernel_hashes.json"


def current_hashes() -> dict:
    files = sorted(GPU_DIR.glob("*Kernels.h"))
    if not files:
        sys.exit(f"error: no *Kernels.h found under {GPU_DIR}")
    return {
        str(f.relative_to(REPO_ROOT)).replace("\\", "/"): hashlib.sha256(f.read_bytes()).hexdigest()
        for f in files
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--update", action="store_true", help="rewrite the manifest from the working tree")
    args = ap.parse_args()

    actual = current_hashes()

    if args.update:
        MANIFEST.write_text(json.dumps(actual, indent=2) + "\n", encoding="utf-8")
        print(f"wrote {MANIFEST.relative_to(REPO_ROOT)} ({len(actual)} kernel headers)")
        return 0

    if not MANIFEST.exists():
        sys.exit(f"error: {MANIFEST} missing - run with --update to create it")

    expected = json.loads(MANIFEST.read_text(encoding="utf-8"))
    problems = []
    for path in sorted(set(expected) | set(actual)):
        if path not in actual:
            problems.append(f"REMOVED   {path}")
        elif path not in expected:
            problems.append(f"UNTRACKED {path} (run --update if intentional)")
        elif expected[path] != actual[path]:
            problems.append(f"MODIFIED  {path}")

    if problems:
        print("kernel-source hash check FAILED:")
        for p in problems:
            print(f"  {p}")
        print("Kernel text is a byte-identical contract (NVRTC + hipRTC + baselines).")
        print("If the change is intentional: rerun GPU smoke baselines and commit --update.")
        return 1

    print(f"kernel-source hash check OK ({len(actual)} headers unchanged)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
