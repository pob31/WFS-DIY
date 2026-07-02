#!/usr/bin/env python3
"""One-time migration: add/refresh the explicit `Tier` column in the WFS-UI CSVs.

Snapshots each parameter row's *currently effective* MCP confirmation tier
(tool_tier_overrides.json -> keyword/dB-span heuristic) into a new `Tier`
column placed immediately after `Default`. After this runs, the generator reads
the column as the primary source (see generate_mcp_tools.process_row); the
override file + heuristic remain only as fallbacks. Because the snapshot equals
today's effective tiers, regenerating generated_tools.json leaves the tools[]
array byte-identical (only the top-level input_hash changes).

Reuses generate_mcp_tools's own row-parsing, family pre-pass, override lookup
and heuristic so the snapshot is byte-exact with the generator.

Idempotent: re-running overwrites an existing `Tier` column in place rather than
inserting a second one. Rewriting is line-oriented (split on '\t', insert one
field) so existing cells and each file's line endings are preserved verbatim.

Run from the repo root:  python tools/mcp/populate_tier_column.py
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

# Make the sibling generator importable.
GENERATOR_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(GENERATOR_DIR))
import generate_mcp_tools as g  # type: ignore  # noqa: E402

DEFAULT_AFTER_COLUMN = "Default"
TIER_HEADER = "Tier"


def _row_from_fields(fields: list[str], col_map: dict[int, str],
                     fname: str) -> g.CSVRow:
    """Build a CSVRow from a raw split line using the header-derived col_map —
    the same mapping generate_mcp_tools.read_csv applies."""
    obj = g.CSVRow(csv_file=fname)
    for i, val in enumerate(fields):
        attr = col_map.get(i)
        if attr is not None:
            setattr(obj, attr, val)
    return obj


def _effective_tier(row: g.CSVRow,
                    tier_overrides: dict[str, int],
                    ignore_map: dict[str, str]) -> int | None:
    """Return the tier this row would resolve to under the pre-column logic
    (override -> heuristic), or None for rows that never become a tool (blank
    variable, non-parameter UI row, or explicitly ignored) — those get a blank
    Tier cell."""
    if not row.variable.strip():
        return None
    if g.is_non_parameter_row(row):
        return None
    if g.is_ignored(row.variable, ignore_map):
        return None
    family = g.detect_numeric_family(row.variable)
    band = g.detect_band_placeholder(row.variable)
    fam_stem = family[0] if family is not None else None
    tier = g.lookup_tier_override(row.variable, fam_stem, band, tier_overrides)
    if tier is None:
        tier = g.heuristic_tier(row.variable, row.label, row.type,
                                row.min, row.max)
    return tier


def populate_file(path: Path,
                  tier_overrides: dict[str, int],
                  ignore_map: dict[str, str]) -> tuple[int, dict[int, int]]:
    """Add/refresh the Tier column in one CSV. Returns (rows_written, dist)."""
    raw = path.read_bytes()
    newline = "\r\n" if b"\r\n" in raw else "\n"
    text = raw.decode("utf-8")
    lines = text.split(newline)

    headers = lines[0].split("\t")
    if TIER_HEADER in headers:
        tier_idx = headers.index(TIER_HEADER)
        inserting = False
    else:
        tier_idx = headers.index(DEFAULT_AFTER_COLUMN) + 1
        inserting = True

    # col_map from the *original* header (before any insertion), matching
    # read_csv. Used to compute each row's tier from its original field layout.
    col_map: dict[int, str] = {}
    for i, h in enumerate(headers):
        attr = g.HEADER_ALIASES.get(h.strip())
        if attr is not None:
            col_map[i] = attr

    out_lines: list[str] = []
    # Header line.
    if inserting:
        headers.insert(tier_idx, TIER_HEADER)
    out_lines.append("\t".join(headers))

    rows_written = 0
    dist: dict[int, int] = {}
    for ln in lines[1:]:
        if ln == "":
            out_lines.append(ln)  # preserve trailing/blank lines verbatim
            continue
        fields = ln.split("\t")
        row = _row_from_fields(fields, col_map, path.name)
        tier = _effective_tier(row, tier_overrides, ignore_map)
        tier_str = str(tier) if tier is not None else ""
        if tier is not None:
            rows_written += 1
            dist[tier] = dist.get(tier, 0) + 1
        # Guard against the (here-absent) ragged short line.
        while len(fields) < tier_idx:
            fields.append("")
        if inserting:
            fields.insert(tier_idx, tier_str)
        else:
            while len(fields) <= tier_idx:
                fields.append("")
            fields[tier_idx] = tier_str
        out_lines.append("\t".join(fields))

    path.write_bytes(newline.join(out_lines).encode("utf-8"))
    return rows_written, dist


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--csv-dir", default="Documentation",
                        help="Directory containing the WFS-UI_*.csv files.")
    parser.add_argument("--overrides-tier",
                        default="tools/mcp/tool_tier_overrides.json")
    parser.add_argument("--overrides-ignore",
                        default="tools/mcp/tool_generation_ignores.json")
    args = parser.parse_args(argv)

    csv_dir = Path(args.csv_dir)
    if not csv_dir.is_dir():
        print(f"error: csv-dir not found: {csv_dir}", file=sys.stderr)
        return 2

    tier_overrides = g.load_overrides_tier(Path(args.overrides_tier))
    ignore_map = g.load_overrides_ignore(Path(args.overrides_ignore))

    # Family pre-pass so numeric-suffix wildcard overrides resolve identically
    # to the generator (must run before detect_numeric_family is called).
    g._REAL_FAMILY_STEMS = g.compute_real_family_stems(csv_dir)

    grand = 0
    for fname in g.CSV_FILES_ORDER:
        p = csv_dir / fname
        if not p.exists():
            print(f"  skip (missing): {fname}")
            continue
        rows, dist = populate_file(p, tier_overrides, ignore_map)
        grand += rows
        dist_str = ", ".join(f"tier{t}={dist.get(t, 0)}" for t in (1, 2, 3))
        print(f"  {fname}: {rows} rows tiered ({dist_str})")
    print(f"done: {grand} parameter rows tiered across {len(g.CSV_FILES_ORDER)} CSVs")
    return 0


if __name__ == "__main__":
    sys.exit(main())
