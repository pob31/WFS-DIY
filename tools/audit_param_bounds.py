#!/usr/bin/env python3
"""Audit parameter bounds across three sources of truth.

Produces a Markdown-format report on stdout with three sections:

1. **Bounds drift** — CSV documents bounds [a, b] but
   WFSParameterDefaults.h or OSCParameterBounds binding says different.
   Most actionable. Each entry shows all three values so the user can
   pick the canonical source.

2. **Missing C++ bindings** — the CSV documents a parameter with
   numeric bounds, but no `<name>Min`/`<name>Max` constants exist in
   WFSParameterDefaults.h, OR no entry in OSCParameterBounds.cpp.
   The OSC range gate will silently pass any value for these.

3. **Suspect UI literals** — hardcoded numeric ranges in `Source/gui/`
   that look like they apply to a known parameter but don't match the
   CSV bounds. Best-effort regex scan; expect false positives. The
   user verifies manually.

Run from repo root:
    python tools/audit_param_bounds.py
"""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

# Reuse the existing CSV parser so we stay consistent with the MCP
# generator's view of the documentation.
sys.path.insert(0, str(Path(__file__).parent))
from generate_mcp_tools import CSVRow, read_csv  # type: ignore


REPO_ROOT = Path(__file__).resolve().parent.parent
CSV_DIR = REPO_ROOT / "Documentation"
DEFAULTS_HEADER = REPO_ROOT / "Source/Parameters/WFSParameterDefaults.h"
PARAM_IDS_HEADER = REPO_ROOT / "Source/Parameters/WFSParameterIDs.h"
OSC_BOUNDS_CPP = REPO_ROOT / "Source/Network/OSCParameterBounds.cpp"
GUI_DIR = REPO_ROOT / "Source/gui"

CSV_FILES = [
    "WFS-UI_config.csv",
    "WFS-UI_input.csv",
    "WFS-UI_output.csv",
    "WFS-UI_reverb.csv",
    "WFS-UI_clusters.csv",
    "WFS-UI_network.csv",
    "WFS-UI_audioPatch.csv",
]


# ---------------------------------------------------------------------------
# Source 1: CSV
# ---------------------------------------------------------------------------

@dataclass
class CsvBound:
    variable: str
    csv_file: str
    type_: str          # "FLOAT", "INT", "STRING", ...
    min_v: float | None
    max_v: float | None
    label: str
    ui: str             # the UI element kind ("H Slider", "Number box", "drop down menu", ...)


def parse_csv_bounds() -> dict[str, CsvBound]:
    """Map variable name -> CsvBound. Skips rows without a variable
    name or rows whose type is not numeric."""
    out: dict[str, CsvBound] = {}
    for filename in CSV_FILES:
        path = CSV_DIR / filename
        if not path.exists():
            continue
        for row in read_csv(path):
            v = (row.variable or "").strip()
            if not v:
                continue
            t = (row.type or "").strip().upper()
            if t not in ("FLOAT", "INT"):
                continue
            mn = _parse_num(row.min)
            mx = _parse_num(row.max)
            if mn is None and mx is None:
                continue
            # Last write wins; the same variable can appear in multiple
            # rows for array-style enumeration. We assume bounds are
            # consistent across those rows.
            out[v] = CsvBound(
                variable=v,
                csv_file=filename,
                type_=t,
                min_v=mn,
                max_v=mx,
                label=(row.label or "").strip(),
                ui=(row.ui or "").strip(),
            )
    return out


def _parse_num(s: str) -> float | None:
    if s is None:
        return None
    s = s.strip()
    if not s:
        return None
    try:
        return float(s)
    except ValueError:
        return None


# ---------------------------------------------------------------------------
# Source 2a: WFSParameterDefaults.h
# ---------------------------------------------------------------------------

@dataclass
class HeaderBound:
    name: str           # the constant prefix, e.g. "stageWidth" for stageWidthMin/Max
    min_v: float | None
    max_v: float | None
    min_line: int | None = None
    max_line: int | None = None


# Matches lines like:
#   constexpr float stageWidthMin = -50.0f;
#   constexpr int   stageShapeMax = 2;
#   inline const float fooMin = 1.0f;
HEADER_CONST_RE = re.compile(
    r"\b(?:constexpr|inline\s+const)\s+(?:float|int|double|long)\s+"
    r"(\w+?)(Min|Max)\s*=\s*([-+]?\d+(?:\.\d+)?(?:[eE][-+]?\d+)?)f?\s*;"
)


def parse_header_bounds() -> dict[str, HeaderBound]:
    """Map prefix name -> HeaderBound. Both Min and Max must exist
    or the entry is omitted."""
    text = DEFAULTS_HEADER.read_text(encoding="utf-8")
    partial: dict[str, dict[str, tuple[float, int]]] = {}
    for line_no, line in enumerate(text.splitlines(), start=1):
        m = HEADER_CONST_RE.search(line)
        if not m:
            continue
        name, suffix, value = m.group(1), m.group(2), m.group(3)
        partial.setdefault(name, {})[suffix] = (float(value), line_no)

    out: dict[str, HeaderBound] = {}
    for name, parts in partial.items():
        if "Min" in parts and "Max" in parts:
            mn, mn_line = parts["Min"]
            mx, mx_line = parts["Max"]
            out[name] = HeaderBound(name, mn, mx, mn_line, mx_line)
        # Skip entries that have only one of the pair — they're not
        # bound-pairs (e.g. stageWidthMax-only would be malformed; we
        # don't enforce single-sided "ranges").
    return out


# ---------------------------------------------------------------------------
# Source 2b: OSCParameterBounds.cpp — paramId -> constant-prefix mapping
# ---------------------------------------------------------------------------

# Matches lines like:
#   BIND_F (stageWidth);
#   BIND_I (inputCluster);
#   BIND_F_AS (inputPositionX, inputPosition);
#   BIND_I_AS (clusterLFOshapeX, inputLFOshape);
#   BIND_BOOL (inputFlipX);
BIND_RE = re.compile(
    r"\b(BIND_F|BIND_I|BIND_F_AS|BIND_I_AS|BIND_BOOL)\s*\(\s*"
    r"(\w+)\s*(?:,\s*(\w+))?\s*\)"
)


@dataclass
class GateBinding:
    param_id: str
    constant_prefix: str | None  # None for BIND_BOOL (uses {0,1} explicit)
    kind: str                     # "F", "I", "F_AS", "I_AS", "BOOL"


def parse_gate_bindings() -> dict[str, GateBinding]:
    """Map paramId -> GateBinding from OSCParameterBounds.cpp."""
    text = OSC_BOUNDS_CPP.read_text(encoding="utf-8")
    out: dict[str, GateBinding] = {}
    for m in BIND_RE.finditer(text):
        macro = m.group(1)
        param_id = m.group(2)
        prefix = m.group(3) if m.group(3) else None
        if macro == "BIND_BOOL":
            kind = "BOOL"
            prefix = None
        elif macro in ("BIND_F", "BIND_I"):
            kind = "F" if macro == "BIND_F" else "I"
            prefix = param_id
        elif macro == "BIND_F_AS":
            kind = "F_AS"
        elif macro == "BIND_I_AS":
            kind = "I_AS"
        else:
            continue
        out[param_id] = GateBinding(param_id, prefix, kind)
    return out


# ---------------------------------------------------------------------------
# Source 3: GUI — best-effort literal scan
# ---------------------------------------------------------------------------

# Two patterns we'll look for on a single line:
#   setRange (lo, hi, ...)
#   jlimit (lo, hi, value)
#   .setRange (lo, hi)
GUI_RANGE_RE = re.compile(
    r"\b(setRange|jlimit)\s*\(\s*"
    r"([-+]?\d+(?:\.\d+)?(?:[eE][-+]?\d+)?)f?\s*,\s*"
    r"([-+]?\d+(?:\.\d+)?(?:[eE][-+]?\d+)?)f?"
)


@dataclass
class GuiHit:
    file: Path
    line_no: int
    func: str          # "setRange" or "jlimit"
    lo: float
    hi: float
    context: str       # short snippet for the report


def scan_gui_literals() -> list[GuiHit]:
    """Walk Source/gui/ and collect every line with a setRange/jlimit
    call whose first two args are numeric literals. Used to flag
    literals that don't agree with the CSV/Header bounds for the same
    parameter (matched heuristically by variable name in the same line
    or its 5-line neighbourhood)."""
    hits: list[GuiHit] = []
    if not GUI_DIR.exists():
        return hits
    for path in sorted(GUI_DIR.rglob("*.h")) + sorted(GUI_DIR.rglob("*.cpp")):
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        for line_no, line in enumerate(text.splitlines(), start=1):
            m = GUI_RANGE_RE.search(line)
            if not m:
                continue
            func, lo_s, hi_s = m.group(1), m.group(2), m.group(3)
            try:
                lo = float(lo_s)
                hi = float(hi_s)
            except ValueError:
                continue
            hits.append(GuiHit(
                file=path,
                line_no=line_no,
                func=func,
                lo=lo,
                hi=hi,
                context=line.strip()[:140],
            ))
    return hits


def find_variable_in_neighbourhood(hit: GuiHit, variables: set[str],
                                    radius: int = 5) -> str | None:
    """Read the file around the hit's line and return the first
    variable name from `variables` that appears within +/- radius
    lines. Lets us heuristically associate a setRange call with a
    parameter."""
    try:
        lines = hit.file.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return None
    lo = max(0, hit.line_no - 1 - radius)
    hi = min(len(lines), hit.line_no + radius)
    window = "\n".join(lines[lo:hi])
    # Prefer the variable closest in name length (less chance of
    # substring collision e.g. inputPosition vs inputPositionX).
    matches = [v for v in variables if re.search(rf"\b{re.escape(v)}\b", window)]
    if not matches:
        return None
    matches.sort(key=lambda s: -len(s))
    return matches[0]


# ---------------------------------------------------------------------------
# Cross-reference and report
# ---------------------------------------------------------------------------

def fmt(v: float | None) -> str:
    if v is None:
        return "—"
    if v == int(v):
        return str(int(v))
    return f"{v:g}"


def main() -> int:
    # Force UTF-8 stdout on Windows so the report can use Unicode arrows.
    if hasattr(sys.stdout, "reconfigure"):
        try:
            sys.stdout.reconfigure(encoding="utf-8")
        except Exception:
            pass

    csv_bounds = parse_csv_bounds()
    header_bounds = parse_header_bounds()
    gate_bindings = parse_gate_bindings()

    # ===== Section 1: bounds drift =====
    print("# Parameter bounds audit\n")
    print(f"_CSV variables with numeric bounds:_ **{len(csv_bounds)}**")
    print(f"_C++ Min/Max constant pairs:_       **{len(header_bounds)}**")
    print(f"_OSCParameterBounds bindings:_       **{len(gate_bindings)}**\n")

    drifts: list[tuple[str, CsvBound, HeaderBound, GateBinding | None]] = []
    missing_header: list[CsvBound] = []
    missing_gate: list[CsvBound] = []
    orphan_header: list[str] = []

    for var, cb in sorted(csv_bounds.items()):
        gate = gate_bindings.get(var)
        prefix = None
        if gate is not None:
            prefix = gate.constant_prefix
        # If gate maps to a different prefix, look up that prefix.
        # Otherwise try the variable name itself.
        if prefix is None and gate is None:
            prefix = var  # try direct name match
        elif prefix is None and gate is not None and gate.kind == "BOOL":
            # BIND_BOOL params are explicit {0,1}; CSV INT 0..1 should match.
            if cb.min_v == 0.0 and cb.max_v == 1.0:
                continue
            else:
                # CSV says non-{0,1} but binding says BOOL — drift.
                drifts.append((var, cb, HeaderBound("(BIND_BOOL)", 0, 1), gate))
                continue

        hb = header_bounds.get(prefix) if prefix else None
        if hb is None:
            missing_header.append(cb)
            if gate is None:
                missing_gate.append(cb)
            continue
        if hb.min_v != cb.min_v or hb.max_v != cb.max_v:
            drifts.append((var, cb, hb, gate))
        if gate is None:
            missing_gate.append(cb)

    # Header constants that aren't tied to any CSV row.
    csv_prefixes_in_use: set[str] = set()
    for var, cb in csv_bounds.items():
        gate = gate_bindings.get(var)
        if gate and gate.constant_prefix:
            csv_prefixes_in_use.add(gate.constant_prefix)
        else:
            csv_prefixes_in_use.add(var)
    for prefix in header_bounds:
        if prefix not in csv_prefixes_in_use:
            orphan_header.append(prefix)

    # ----- 1. Drift -----
    print("## 1. CSV ↔ C++ bounds drift\n")
    if not drifts:
        print("None found. ✅\n")
    else:
        print(f"**{len(drifts)} drift(s) found.**\n")
        print("| Variable | CSV file | CSV [min, max] | Header [min, max] | Header line |")
        print("|---|---|---|---|---|")
        for var, cb, hb, gate in drifts:
            csv_range = f"[{fmt(cb.min_v)}, {fmt(cb.max_v)}]"
            hdr_range = f"[{fmt(hb.min_v)}, {fmt(hb.max_v)}]"
            line_ref = f"L{hb.min_line}/{hb.max_line}" if hb.min_line else "-"
            print(f"| `{var}` | {cb.csv_file} | {csv_range} | {hdr_range} | {line_ref} |")
        print()

    # ----- 2. Missing -----
    print("## 2. CSV-documented bounds with no C++ enforcement\n")
    if not missing_gate:
        print("None — every numeric CSV row is covered by OSCParameterBounds. ✅\n")
    else:
        print(f"**{len(missing_gate)} parameter(s) have bounds in the CSV but no entry in the OSC range gate.** "
              "These pass through unchecked.\n")
        print("| Variable | CSV file | CSV [min, max] | Type |")
        print("|---|---|---|---|")
        for cb in sorted(missing_gate, key=lambda c: c.variable):
            csv_range = f"[{fmt(cb.min_v)}, {fmt(cb.max_v)}]"
            print(f"| `{cb.variable}` | {cb.csv_file} | {csv_range} | {cb.type_} |")
        print()

    # ----- 3. Orphan header constants -----
    if orphan_header:
        print("## 3. C++ Min/Max constants with no matching CSV row\n")
        print(f"**{len(orphan_header)} constant pair(s) with no CSV entry.** "
              "Could be legit shared bases (e.g. `inputPosition`) or stale.\n")
        for name in sorted(orphan_header):
            hb = header_bounds[name]
            print(f"- `{name}Min/Max` = [{fmt(hb.min_v)}, {fmt(hb.max_v)}] (line {hb.min_line})")
        print()

    # ----- 4. Suspect UI literals -----
    print("## 4. GUI hardcoded ranges that may disagree\n")
    print("_Best-effort regex scan of `Source/gui/`. Lines with `setRange(lo, hi)` "
          "or `jlimit(lo, hi, value)` whose `(lo, hi)` doesn't match the CSV bounds for a "
          "parameter mentioned within ±5 lines. Manual verification required._\n")

    var_names = set(csv_bounds.keys())
    gui_hits = scan_gui_literals()
    suspect: list[tuple[GuiHit, str, CsvBound]] = []
    for hit in gui_hits:
        var = find_variable_in_neighbourhood(hit, var_names)
        if var is None:
            continue
        cb = csv_bounds[var]
        if cb.min_v is None or cb.max_v is None:
            continue
        if hit.lo == cb.min_v and hit.hi == cb.max_v:
            continue
        suspect.append((hit, var, cb))

    if not suspect:
        print("None. ✅\n")
    else:
        print(f"**{len(suspect)} suspect literal(s) flagged.** False positives expected for "
              "sliders that operate in normalised 0-1 space and apply scaling at the "
              "formula site rather than via setRange.\n")
        print("| Variable | GUI file:line | GUI [lo, hi] | CSV [min, max] | Context |")
        print("|---|---|---|---|---|")
        for hit, var, cb in suspect:
            rel = hit.file.relative_to(REPO_ROOT).as_posix()
            gui_range = f"[{fmt(hit.lo)}, {fmt(hit.hi)}]"
            csv_range = f"[{fmt(cb.min_v)}, {fmt(cb.max_v)}]"
            ctx = hit.context.replace("|", "\\|")
            print(f"| `{var}` | {rel}:{hit.line_no} | {gui_range} | {csv_range} | `{ctx}` |")
        print()

    # ----- summary footer -----
    print("---")
    print(f"_Drifts:_ {len(drifts)}  ·  "
          f"_Missing gate entries:_ {len(missing_gate)}  ·  "
          f"_Orphan header constants:_ {len(orphan_header)}  ·  "
          f"_Suspect UI literals:_ {len(suspect)}")
    return 0 if not drifts else 1


if __name__ == "__main__":
    raise SystemExit(main())
