#!/usr/bin/env python3
"""App-agnostic MCP tool-surface generator core (spatcore Phase 4d).

Reads tab-separated parameter CSVs and emits a deterministic MCP tool
manifest (`generated_tools.json`) plus a group-key lookup
(`generated_groups.json`). This module is pure mechanism:

  - tab-CSV -> CSVRow reader with header aliasing;
  - CamelCase -> snake_case naming pipeline (abbreviation expansion is
    table-driven);
  - numeric-suffix family collapse (`fooAtten1..10` -> one tool with an
    index arg) with a sister-row pre-scan gate;
  - tier precedence: explicit CSV `Tier` column > override file >
    keyword/range heuristic (which warns loudly when it fires);
  - JSON-Schema derivation, tier-2/3 confirm-handshake injection,
    validation, and a deterministic sorted/input-hashed JSON writer.

All app vocabulary (CSV namespaces, abbreviation maps, channel-id
ranges, tier keyword tables, OSC path conventions, domain taxonomy,
input/output paths) is injected via `configure(config)` before use —
the core defines none of it. WFS-DIY's configuration lives in
`tools/mcp/wfs_codegen_config.py` and is installed by the thin
entry-point wrapper `tools/generate_mcp_tools.py` (same CLI as before
the split). Spec: Documentation/MCP/specs/GENERATION_SCRIPT_SPEC.md.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Iterable

SCHEMA_VERSION = "1.0"

# ------------------------------------------------------------- configuration
#
# The app config (a module or any attribute-bearing object) must provide
# every name in _CONFIG_KEYS. `configure()` installs them as module
# globals so the transform functions below read them exactly as the
# pre-split monolith did. See tools/mcp/wfs_codegen_config.py for the
# WFS-DIY tables and per-table documentation.

_CONFIG_KEYS = (
    # CSV file -> tool namespace prefix used in tool names and group_keys.
    "CSV_NAMESPACE",
    # OSC-path convention for CSVs that don't carry an explicit path column.
    "OSC_PATH_CONVENTION",
    # Variable-prefix stripping per CSV (longest match wins).
    "VARIABLE_PREFIXES",
    # Ordered (raw, snake_case) acronym expansions for tool naming.
    "ABBREVIATIONS",
    # Word-fragment expansions applied after snake_casing (atten -> attenuation).
    "WORD_EXPANSIONS",
    # Tier heuristic fallback keyword tables + wide-range risk rule.
    "TIER_KEYWORDS_3",
    "TIER_KEYWORDS_2",
    "TIER_RANGE_RISK",
    # Domain-tag taxonomy tables.
    "DOMAIN_VARIABLE_OVERRIDES",
    "DOMAIN_BY_SECTION_KEYWORD",
    "DOMAIN_DEFAULT_BY_CSV",
    # UI-control vocabulary that disqualifies a row from becoming a tool.
    "NON_PARAMETER_UI",
    "NON_PARAMETER_VARIABLES_PREFIXES",
    # Per-CSV channel-id argument: (arg_name, min, max, description).
    "CHANNEL_ID_RANGE",
    # Sub-index conventions: <band> placeholder + band arg, per-family arg
    # rules (csv_file + stem suffix -> named ranged arg), cell-index rules.
    "BAND_PLACEHOLDER",
    "BAND_ARG",
    "FAMILY_ARG_RULES",
    "CELL_INDEX_RULES",
    # Section suffixes stripped before group_key / tool-name derivation,
    # e.g. coordinate-system tags like "(Cylindrical)".
    "COORDINATE_SECTION_SUFFIXES",
    # Hint appended to per-channel tool descriptions.
    "PER_CHANNEL_HINT",
    # Input CSVs in consumption order, and default CLI paths.
    "CSV_FILES_ORDER",
    "DEFAULT_CSV_DIR",
    "DEFAULT_OVERRIDES_TIER",
    "DEFAULT_OVERRIDES_IGNORE",
    "DEFAULT_OUTPUT",
    "DEFAULT_GROUPS_OUTPUT",
)

_CONFIGURED = False

# Compiled in configure() from COORDINATE_SECTION_SUFFIXES.
_COORD_SUFFIX_RE: re.Pattern[str] | None = None


def configure(config: Any) -> None:
    """Install an app configuration. `config` is any object (typically a
    module) exposing every attribute named in `_CONFIG_KEYS`."""
    global _CONFIGURED, _COORD_SUFFIX_RE
    missing = [k for k in _CONFIG_KEYS if not hasattr(config, k)]
    if missing:
        raise RuntimeError(
            f"codegen config is missing required keys: {', '.join(missing)}")
    g = globals()
    for k in _CONFIG_KEYS:
        g[k] = getattr(config, k)
    _COORD_SUFFIX_RE = re.compile(
        r"\s*\(("
        + "|".join(re.escape(s) for s in COORDINATE_SECTION_SUFFIXES)
        + r")\)\s*$")
    _CONFIGURED = True


# Variable-suffix -> short label substitutions for nudge-tool naming.
NUDGE_VERB = "nudge"
SET_VERB = "set"


def derive_domains(row: CSVRow, csv_namespace: str) -> list[str]:
    """Resolve the domain tags for a single CSV row.

    Resolution order (first match wins):
      1. Variable-name override.
      2. Section-keyword match scoped to the CSV namespace.
      3. Per-CSV fallback.
    """
    if row.variable in DOMAIN_VARIABLE_OVERRIDES:
        return list(DOMAIN_VARIABLE_OVERRIDES[row.variable])

    section_lower = row.section.lower().strip()
    for ns, keyword, domains in DOMAIN_BY_SECTION_KEYWORD:
        if ns != csv_namespace:
            continue
        if keyword == "" or keyword in section_lower:
            return list(domains)

    return list(DOMAIN_DEFAULT_BY_CSV.get(row.csv_file, []))


@dataclass
class CSVRow:
    csv_file: str
    section: str = ""
    label: str = ""
    variable: str = ""
    ui: str = ""
    type: str = ""
    min: str = ""
    max: str = ""
    default: str = ""
    tier: str = ""
    formula: str = ""
    unit: str = ""
    enum: str = ""
    notes: str = ""
    osc_path: str = ""
    osc_inc_dec: str = ""
    osc_optional_value: str = ""
    osc_remote_path: str = ""
    hover: str = ""
    keyboard: str = ""
    array_value: str = ""
    raw: dict[str, str] = field(default_factory=dict)


# Header → CSVRow attribute name. Multiple aliases allowed.
HEADER_ALIASES = {
    "Section":  "section",
    "Label":    "label",
    "Variable": "variable",
    "UI":       "ui",
    "Type":     "type",
    "Min":      "min",
    "Max":      "max",
    "Default":  "default",
    "Tier":     "tier",
    "Formula for UI elements (x from 0.0 to 1.0)": "formula",
    "Unit":     "unit",
    "enum":     "enum",
    "Notes":    "notes",
    "Array value": "array_value",
    "OSC path": "osc_path",
    "OSC \"inc\" or \"dec\" before value to increment of decrement": "osc_inc_dec",
    "OSC path optional value": "osc_optional_value",
    "OSC remote path": "osc_remote_path",
    "Hover help text in the status bar": "hover",
    "Keyboard shortcuts": "keyboard",
}


def read_csv(path: Path) -> Iterable[CSVRow]:
    """Yield CSVRow objects from a tab-separated CSV file with a header row."""
    with path.open(encoding="utf-8", newline="") as f:
        reader = csv.reader(f, delimiter="\t")
        try:
            headers = next(reader)
        except StopIteration:
            return
        # Build a column-name -> attribute-name map.
        col_map: dict[int, str] = {}
        for i, h in enumerate(headers):
            attr = HEADER_ALIASES.get(h.strip())
            if attr is not None:
                col_map[i] = attr
        for row in reader:
            # Skip totally blank rows.
            if not any(c.strip() for c in row):
                continue
            obj = CSVRow(csv_file=path.name)
            for i, val in enumerate(row):
                attr = col_map.get(i)
                if attr is None:
                    continue
                setattr(obj, attr, val)
            obj.raw = {h: row[i] if i < len(row) else "" for i, h in enumerate(headers)}
            yield obj


# -------------------------------------------------------------------- naming

def camel_to_snake(name: str) -> str:
    """Convert CamelCase / camelCase to snake_case. Lowercase chars and
    underscores pass through unchanged, so a partially-snake_cased input
    (the typical output of expand_abbreviations) finishes the conversion
    cleanly."""
    out = []
    for i, ch in enumerate(name):
        if ch.isupper() and i > 0:
            prev = name[i-1]
            nxt = name[i+1] if i+1 < len(name) else ""
            if prev.islower() or prev.isdigit() or (nxt and nxt.islower()):
                if out and out[-1] != "_":
                    out.append("_")
        out.append(ch.lower())
    s = "".join(out)
    s = re.sub(r"_+", "_", s).strip("_")
    return s


def expand_abbreviations(variable_stripped: str) -> str:
    """Replace known acronym fragments with their snake_case forms in the
    CamelCase variable name. Two-pass: this pass produces a mix of
    snake_case (where abbreviations matched) and CamelCase (residual);
    the caller then runs `camel_to_snake` to finish converting the
    residual."""
    s = variable_stripped
    if not s:
        return s
    items = sorted(ABBREVIATIONS, key=lambda x: -len(x[0]))
    out_parts: list[str] = []
    i = 0
    while i < len(s):
        matched = False
        for raw, expanded in items:
            if not s[i:].startswith(raw):
                continue
            # Word-start boundary check.
            ok_before = (i == 0)
            if not ok_before:
                prev = s[i-1]
                if not prev.isalnum():
                    ok_before = True
                elif prev.islower() and raw[0].isupper():
                    ok_before = True  # CamelCase case transition
                elif prev.isupper() and raw[0].isupper():
                    ok_before = True  # mid-acronym continuation
                elif prev.isdigit():
                    ok_before = True
            if not ok_before:
                continue
            # Word-end boundary check.
            end = i + len(raw)
            ok_after = (end == len(s))
            if not ok_after:
                nxt = s[end]
                if not nxt.isalpha() or nxt.isupper() or nxt.isdigit():
                    ok_after = True
            if not ok_after:
                continue
            # Emit a separator if the previous char was alphanumeric.
            if out_parts and out_parts[-1] and out_parts[-1][-1].isalnum():
                out_parts.append("_")
            out_parts.append(expanded)
            i = end
            matched = True
            break
        if matched:
            continue
        out_parts.append(s[i])
        i += 1
    return "".join(out_parts)


def variable_to_snake(s: str) -> str:
    """Full pipeline: expand abbreviations, then camel_to_snake, then expand
    common word fragments (atten -> attenuation, freq -> frequency, ...)."""
    s = expand_abbreviations(s)
    s = camel_to_snake(s)
    if not s:
        return s
    parts = s.split("_")
    parts = [WORD_EXPANSIONS.get(p, p) for p in parts]
    return "_".join(parts)


def strip_variable_prefix(variable: str, csv_file: str) -> str:
    """Strip the longest matching CSV-specific prefix from the variable."""
    for prefix in sorted(VARIABLE_PREFIXES.get(csv_file, []),
                         key=len, reverse=True):
        if variable.startswith(prefix):
            rest = variable[len(prefix):]
            if rest and (rest[0].isupper() or rest[0].isdigit()):
                return rest
    return variable


# Variables we know carry a numeric suffix family. Detect at the level of a
# constant trailing digit-run, then a sister entry confirming this is a family.
_NUMERIC_FAMILY_RE = re.compile(r"^(.*?)(\d+)$")

# Populated once at the top of `main()` after a pre-scan of every CSV.
# Holds the set of stems that actually appear in two-or-more sister rows
# (e.g. `inputArrayAtten` for inputArrayAtten1..10). Without this gate,
# a lone variable whose name happens to end in digits — like `reverbRT60`
# — is treated as "instance #60 of a 60-element family" and silently
# skipped because num != 1. With the gate, only stems with real siblings
# trigger the family-dedup path.
_REAL_FAMILY_STEMS: set[str] | None = None


def detect_numeric_family(variable: str) -> tuple[str, int] | None:
    """If `variable` ends in a digit AND its stem has sister rows, return
    (stem, number); else None. Lone numeric-suffix variables (e.g.
    `reverbRT60`, `mp3Bitrate`) return None and become regular tools."""
    m = _NUMERIC_FAMILY_RE.match(variable)
    if not m:
        return None
    stem, num = m.group(1), int(m.group(2))
    if _REAL_FAMILY_STEMS is not None and stem not in _REAL_FAMILY_STEMS:
        return None
    return stem, num


def compute_real_family_stems(csv_dir: Path) -> set[str]:
    """Pre-scan every CSV and return the set of numeric-suffix stems that have
    ≥ 2 sister variables sharing the same stem (e.g. `inputArrayAtten` for
    inputArrayAtten1..10). Without this gate, a lone parameter whose name ends
    in digits — such as `reverbRT60` — is mistakenly treated as the 60th member
    of a 60-element family and silently skipped by process_row's family dedup.

    Used by both main() and tools/mcp/populate_tier_column.py so the family
    detection resolves identically in each.
    """
    stems_seen: dict[str, set[int]] = defaultdict(set)
    for fname in CSV_FILES_ORDER:
        p = csv_dir / fname
        if not p.exists():
            continue
        for row in read_csv(p):
            if not row.variable.strip():
                continue
            m = _NUMERIC_FAMILY_RE.match(row.variable)
            if m:
                stems_seen[m.group(1)].add(int(m.group(2)))
    return {s for s, nums in stems_seen.items() if len(nums) >= 2}


def detect_band_placeholder(variable: str) -> bool:
    return BAND_PLACEHOLDER in variable


# -------------------------------------------------------------------- enums

# Pattern for "Label Name (N)" enum entries.
_ENUM_WITH_ID_RE = re.compile(r"^(.*?)\s*\((\d+)\)\s*$")


def parse_enum(enum_str: str) -> tuple[list[str], dict[str, int] | None]:
    """Parse an `enum` cell. Returns (items, string_to_int_map_or_None).

    Defensive heuristic: only treat the cell as an enum when it holds
    multiple `;`-separated items, OR at least one entry uses the explicit
    `Label (N)` form. A lone string in the cell is almost always a stray
    comment/note that drifted from the Notes column (one such drift
    produced `enum: ["VirtualparamconvertstoXYZinternally"]` for the
    AutomOtion virtual-radius tool). Better to surface the value as a
    real number/string than to ship a single-element bogus enum.

    If items have explicit "(N)" stored IDs, the map is returned. Otherwise
    items are taken at face value and the C++ side maps positionally.
    """
    if not enum_str.strip():
        return [], None
    raw_items = [p.strip() for p in enum_str.split(";")]
    raw_items = [p for p in raw_items if p]
    if not raw_items:
        return [], None
    has_explicit_ids = any(_ENUM_WITH_ID_RE.match(item) for item in raw_items)
    if len(raw_items) < 2 and not has_explicit_ids:
        # Single non-`(N)` entry — almost certainly a stray comment, not a
        # one-value enum. Drop it.
        return [], None
    items: list[str] = []
    mapping: dict[str, int] = {}
    has_ids = False
    for item in raw_items:
        m = _ENUM_WITH_ID_RE.match(item)
        if m:
            label, num = m.group(1).strip(), int(m.group(2))
            has_ids = True
            slug = re.sub(r"[^A-Za-z0-9]", "", label.replace("/", ""))
            items.append(slug if slug else label)
            mapping[items[-1]] = num
        else:
            slug = re.sub(r"[^A-Za-z0-9]", "", item)
            items.append(slug if slug else item)
            mapping[items[-1]] = len(items) - 1
    return items, (mapping if has_ids else None)


# ----------------------------------------------------------------- ignore/tier

def load_overrides_tier(path: Path) -> dict[str, int]:
    if not path.exists():
        return {}
    return json.loads(path.read_text(encoding="utf-8"))


def load_overrides_ignore(path: Path) -> dict[str, str]:
    """Returns variable_name -> reason."""
    if not path.exists():
        return {}
    data = json.loads(path.read_text(encoding="utf-8"))
    out: dict[str, str] = {}
    for entry in data.get("ignored", []):
        out[entry["variable"]] = entry.get("reason", "")
    return out


def is_ignored(variable: str, ignore_map: dict[str, str]) -> str | None:
    if not variable:
        return "(blank Variable cell)"
    if variable in ignore_map:
        return ignore_map[variable]
    return None


def parse_tier_cell(cell: str) -> int | None:
    """Parse the CSV `Tier` cell. Returns 1, 2, or 3 for a valid explicit tier,
    or None if the cell is blank or holds an out-of-range / non-integer value
    (the caller then falls back to the override file / heuristic)."""
    s = (cell or "").strip()
    if not s:
        return None
    try:
        v = int(s)
    except ValueError:
        return None
    return v if v in (1, 2, 3) else None


def lookup_tier_override(variable: str, family_stem: str | None,
                          band_placeholder: bool,
                          overrides: dict[str, int]) -> int | None:
    """Apply override-key lookup: literal, then band-placeholder, then `*`
    wildcard for numeric-suffix families."""
    if variable in overrides:
        return overrides[variable]
    if band_placeholder:
        # The Variable cell already contains the band placeholder; literal
        # lookup above already handles it. Nothing extra here.
        pass
    if family_stem is not None:
        wildcard_key = family_stem + "*"
        if wildcard_key in overrides:
            return overrides[wildcard_key]
    return None


def heuristic_tier(variable: str, label: str, type_: str,
                    min_v: str, max_v: str) -> int:
    """Default tier classification when no override applies.

    Uses word-boundary regex so e.g. `clusterLFOPresetName` doesn't
    accidentally match the "reset" keyword via substring. The label
    half is naturally word-separated; the variable half is CamelCase,
    so we split on lowercase->uppercase transitions before scanning.
    """
    # Split CamelCase variable into space-separated words: "inputOtomoReset"
    # -> "input Otomo Reset". The label is already space-separated.
    var_split = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", " ", variable)
    name = var_split + " " + label
    name_lower = name.lower()
    for kw in TIER_KEYWORDS_3:
        if re.search(r"\b" + re.escape(kw.lower()) + r"\b", name_lower):
            return 3
    for kw in TIER_KEYWORDS_2:
        if re.search(r"\b" + re.escape(kw.lower()) + r"\b", name_lower):
            return 2
    # Wide numeric ranges on risk-flagged names (attenuation/level/... per the
    # app's TIER_RANGE_RISK rule) escalate — sudden loud output risk.
    try:
        if type_.upper() in ("FLOAT", "INT") and min_v and max_v:
            span = float(max_v) - float(min_v)
            if span >= TIER_RANGE_RISK["min_span"] and any(
                    kw in name_lower for kw in TIER_RANGE_RISK["keywords"]):
                return TIER_RANGE_RISK["tier"]
    except ValueError:
        pass
    return 1


# ----------------------------------------------------------------- group_key

def derive_group_key(section: str, variable: str, csv_namespace: str) -> str:
    """Group key used for undo dependency-chasing. Sections normalised to
    snake_case and prefixed with the CSV namespace. Empty sections fall back
    to the variable name."""
    s = _COORD_SUFFIX_RE.sub("", section).strip()
    s = s.replace(".", "")
    if not s:
        # Fallback to a variable-derived singleton key.
        return f"{csv_namespace}_{variable_to_snake(variable)}"
    s = variable_to_snake(re.sub(r"[^A-Za-z0-9]+", "_", s).strip("_"))
    # Strip a redundant csv-namespace prefix (e.g. "cluster_lfo" with
    # csv_namespace="cluster" becomes just "lfo").
    if s == csv_namespace:
        return f"{csv_namespace}"
    if s.startswith(csv_namespace + "_"):
        s = s[len(csv_namespace) + 1:]
    return f"{csv_namespace}_{s}"


# ----------------------------------------------------------------- transform

def is_non_parameter_row(row: CSVRow) -> bool:
    """Drop rows that are documentation-only UI behaviours, not parameters."""
    if not row.variable.strip():
        return True
    if row.variable in NON_PARAMETER_VARIABLES_PREFIXES:
        return True
    ui = row.ui.strip()
    if ui in NON_PARAMETER_UI:
        return True
    return False


def derive_tool_name(row: CSVRow, csv_namespace: str) -> tuple[str, str | None]:
    """Compute the underscore-namespaced tool name and (optionally) the
    matching nudge tool name. Names use only [A-Za-z0-9_-] so they pass
    the OpenAI-style tool-name regex (^[a-zA-Z0-9_-]{1,64}$) that some
    MCP clients (e.g. ChatGPT's Frontend MCP integration) enforce."""
    variable = row.variable
    family = detect_numeric_family(variable)
    band = detect_band_placeholder(variable)

    # Strip the band placeholder for naming purposes
    name_var = variable.replace(BAND_PLACEHOLDER, "")
    # Strip trailing digit family number
    if family is not None and not band:
        stem, _ = family
        name_var = stem

    stripped = strip_variable_prefix(name_var, row.csv_file)
    expanded = variable_to_snake(stripped) if stripped else ""

    # Section → namespace if non-redundant.
    section = _COORD_SUFFIX_RE.sub("", row.section).strip()
    # Strip dots from section names like "L.F.O" so they collapse to "LFO"
    # rather than splitting into "L_F_O" downstream.
    section = section.replace(".", "")
    section_snake = variable_to_snake(re.sub(r"[^A-Za-z0-9]+", "_", section).strip("_")) if section else ""
    # Strip the CSV namespace prefix from the section if present, so e.g.
    # cluster.csv's "Cluster LFO" section becomes just "lfo" rather than
    # "cluster_lfo" (which would render as redundant `cluster.cluster_lfo.*`).
    if section_snake == csv_namespace:
        section_snake = ""
    elif section_snake.startswith(csv_namespace + "_"):
        section_snake = section_snake[len(csv_namespace) + 1:]

    parts = [csv_namespace]
    if section_snake and not _is_redundant_section(section_snake, expanded):
        parts.append(section_snake)
        # Strip the section prefix from the expanded param name if present.
        expanded = _strip_section_prefix(expanded, section_snake)

    base = expanded if expanded else "value"
    parts.append(f"{SET_VERB}_{base}")
    tool_name = "_".join(parts)

    # Nudge variant if the row supports relative writes.
    nudge_name = None
    if row.osc_inc_dec.strip().lower() == "y":
        if section_snake and not _is_redundant_section(section_snake, base):
            nudge_name = f"{csv_namespace}_{section_snake}_{NUDGE_VERB}_{base}"
        else:
            nudge_name = f"{csv_namespace}_{NUDGE_VERB}_{base}"

    return tool_name, nudge_name


def _is_redundant_section(section_snake: str, expanded_param: str) -> bool:
    """If the param name would just repeat the section, drop the section."""
    if not section_snake or not expanded_param:
        return True
    return expanded_param == section_snake


def _strip_section_prefix(expanded: str, section_snake: str) -> str:
    """If the expanded param starts with the section name, strip it."""
    if expanded == section_snake:
        return ""
    if expanded.startswith(section_snake + "_"):
        return expanded[len(section_snake) + 1:]
    return expanded


def derive_description(row: CSVRow, csv_namespace: str,
                        per_channel: bool) -> str:
    base = row.hover.strip() or row.label.strip()
    if not base:
        base = row.variable
    parts = [base.rstrip(".")]
    if row.unit.strip():
        parts.append(f"Value in {row.unit.strip()}.")
    if row.enum.strip():
        parts.append(f"Enum: {row.enum.strip()}.")
    if "transition" in row.osc_optional_value.lower():
        parts.append(
            "Optional `transition_seconds` argument for smooth interpolation."
        )
    if per_channel:
        parts.append(PER_CHANNEL_HINT)
    return ". ".join(p.rstrip(".") for p in parts) + "."


# Tier-marker suffixes appended to tool descriptions so the model sees
# the tier on first read. Keep textually identical to the constants in
# spatcore/control/mcp/MCPToolRegistry.h — both surfaces (auto-gen and
# hand-written) carry the same string so a regex search finds both.
TIER_2_DESCRIPTION_SUFFIX = (
    " [TIER 2: needs a confirm-token round trip OR an open Tier-2 "
    "auto-confirm / safety-gate window.]"
)
TIER_3_DESCRIPTION_SUFFIX = (
    " [TIER 3: destructive - refused unless the operator's safety gate "
    "is open; with the gate open, executes immediately.]"
)


def append_tier_suffix(description: str, tier: int) -> str:
    """Append the tier marker for tier-2 / tier-3 tools. Tier-1 tools
    return unchanged — the absence of a suffix means tier-1 by
    convention. Idempotent: running twice doesn't double the suffix."""
    suffix = ""
    if tier == 2:
        suffix = TIER_2_DESCRIPTION_SUFFIX
    elif tier == 3:
        suffix = TIER_3_DESCRIPTION_SUFFIX
    if suffix and not description.endswith(suffix):
        return description + suffix
    return description


def is_per_channel(row: CSVRow) -> bool:
    return row.csv_file in CHANNEL_ID_RANGE


def coerce_default(default_str: str, type_str: str,
                    enum_items: list[str],
                    enum_map: dict[str, int] | None) -> Any | None:
    """Coerce a CSV `Default` cell into the right JSON-schema value type.

    Returns the value to drop into the schema's `default` slot, or None when
    the cell can't be cleanly coerced (free-form prose like "distribute in
    the middle of the stage", template strings like "input <ID>", or empty).

    For enum-typed value args the schema declares the `value` arg as a
    string with an `enum` list, so the default must be one of those strings.
    Numeric defaults in enum cells are translated through `enum_map` (when
    present, with explicit `Label (N)` IDs in the CSV) or positionally.
    """
    s = (default_str or "").strip()
    if not s:
        return None
    type_norm = type_str.strip().upper()

    if enum_items:
        # Try positional/explicit numeric ID first.
        try:
            num = int(float(s))
        except ValueError:
            num = None
        if num is not None:
            if enum_map is not None:
                # Reverse lookup: which enum_item maps to this stored ID?
                for label, mapped in enum_map.items():
                    if mapped == num:
                        return label
                return None
            if 0 <= num < len(enum_items):
                return enum_items[num]
            return None
        # Maybe the default is already the enum label slug.
        if s in enum_items:
            return s
        return None

    if type_norm.startswith("INT"):
        try:
            return int(float(s))
        except ValueError:
            return None
    if type_norm.startswith("FLOAT"):
        try:
            return float(s)
        except ValueError:
            return None
    if type_norm.startswith("STRING"):
        # Skip template strings that contain placeholders (`<ID>`,
        # `<index>`); they're not valid literal defaults.
        if "<" in s and ">" in s:
            return None
        return s
    if type_norm.startswith("IP"):
        # Validate IPv4 shape; drop garbage.
        if re.match(r"^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$", s):
            return s
        return None
    return None


def derive_schema(row: CSVRow, tool_name: str,
                   enum_items: list[str], enum_map: dict[str, int] | None,
                   family: tuple[str, int] | None,
                   band: bool) -> tuple[dict, list[str], str]:
    """Build the JSON Schema for a tool's `parameters` field.

    Returns (schema, required_list, value_arg_name) — the third item names
    which property carries the actual value (most often "value", but
    "name"/"mode"/"shape"/"protocol" for self-documenting cases). The
    C++ loader uses this to know which JSON key to read for the write."""
    properties: dict[str, dict] = {}
    required: list[str] = []
    value_arg_name = "value"

    # Channel-id argument first (per-channel CSVs).
    if is_per_channel(row):
        cid_name, cid_min, cid_max, cid_desc = CHANNEL_ID_RANGE[row.csv_file]
        properties[cid_name] = {
            "type": "integer",
            "minimum": cid_min,
            "maximum": cid_max,
            "description": cid_desc,
        }
        required.append(cid_name)
    else:
        # App-declared cell-index arguments (e.g. a grid cell selector).
        for rule in CELL_INDEX_RULES:
            if row.csv_file == rule["csv_file"] \
                    and rule["label_substring"] in row.label:
                properties[rule["name"]] = {
                    "type": "integer",
                    "minimum": rule["min"], "maximum": rule["max"],
                    "description": rule["description"],
                }
                required.append(rule["name"])
                break

    # Sub-index argument if applicable.
    if band:
        properties[BAND_ARG["name"]] = {
            "type": "integer",
            "minimum": BAND_ARG["min"], "maximum": BAND_ARG["max"],
            "description": BAND_ARG["description"],
        }
        required.append(BAND_ARG["name"])
    elif family is not None:
        rule = None
        for r in FAMILY_ARG_RULES:
            if row.csv_file == r["csv_file"] \
                    and family[0].lower().endswith(r["stem_suffix"]):
                rule = r
                break
        if rule is not None:
            properties[rule["name"]] = {
                "type": "integer",
                "minimum": rule["min"], "maximum": rule["max"],
                "description": rule["description"],
            }
            required.append(rule["name"])
        else:
            # Generic numeric-suffix family - expose as `index`.
            properties["index"] = {
                "type": "integer", "minimum": 1,
                "description": "Item index (1-based).",
            }
            required.append("index")

    # Value argument.
    type_str = row.type.strip().upper()
    csv_default = coerce_default(row.default, row.type, enum_items, enum_map)
    val_arg: dict[str, Any] = {}
    if enum_items:
        val_name = "value"
        # Pick a more semantic name for the value arg if obvious.
        if "shape" in row.variable.lower():
            val_name = "shape"
        elif "protocol" in row.variable.lower():
            val_name = "protocol"
        elif "mode" in row.variable.lower():
            val_name = "mode"
        val_arg = {
            "type": "string",
            "enum": enum_items,
            "description": f"{row.label.strip() or 'Value'} (enum).",
        }
        if csv_default is not None:
            val_arg["default"] = csv_default
        properties[val_name] = val_arg
        required.append(val_name)
        value_arg_name = val_name
    elif type_str.startswith("INT"):
        val_arg = {"type": "integer"}
        if row.min.strip():
            try:
                val_arg["minimum"] = int(float(row.min))
            except ValueError:
                pass
        if row.max.strip():
            try:
                val_arg["maximum"] = int(float(row.max))
            except ValueError:
                pass
        unit = f" {row.unit.strip()}" if row.unit.strip() else ""
        val_arg["description"] = (row.label.strip() or "Value") + unit + "."
        if csv_default is not None:
            val_arg["default"] = csv_default
        properties["value"] = val_arg
        required.append("value")
    elif type_str.startswith("FLOAT"):
        val_arg = {"type": "number"}
        if row.min.strip():
            try:
                val_arg["minimum"] = float(row.min)
            except ValueError:
                pass
        if row.max.strip():
            try:
                val_arg["maximum"] = float(row.max)
            except ValueError:
                pass
        unit = f" {row.unit.strip()}" if row.unit.strip() else ""
        val_arg["description"] = (row.label.strip() or "Value") + unit + "."
        if csv_default is not None:
            val_arg["default"] = csv_default
        properties["value"] = val_arg
        required.append("value")
    elif type_str.startswith("STRING"):
        val_arg = {
            "type": "string",
            "description": (row.label.strip() or "Value") + ".",
        }
        if csv_default is not None:
            val_arg["default"] = csv_default
        # Self-documenting argument name for rename-style tools. Variables
        # like `outputName`, `samplerSetName`, `clusterLfoPresetName` are
        # recognisable by their `Name` suffix, and matching that to a
        # `name:` argument matches `input.set_name` (the hand-written
        # reference) and the operator's natural phrasing
        # ("rename input 3 to Marie").
        val_name = "name" if row.variable.endswith("Name") else "value"
        properties[val_name] = val_arg
        required.append(val_name)
        value_arg_name = val_name
    elif type_str.startswith("IP"):
        val_arg = {
            "type": "string",
            "pattern": r"^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$",
            "description": (row.label.strip() or "IPv4 address") + ".",
        }
        if csv_default is not None:
            val_arg["default"] = csv_default
        properties["value"] = val_arg
        required.append("value")
    else:
        # Catch-all: the row may not be a settable parameter (button, etc.).
        val_arg = {"type": "string", "description": "Value."}
        if csv_default is not None:
            val_arg["default"] = csv_default
        properties["value"] = val_arg
        required.append("value")

    # Optional transition_seconds argument.
    if "transition" in row.osc_optional_value.lower():
        properties["transition_seconds"] = {
            "type": "number",
            "minimum": 0.0,
            "maximum": 600.0,
            "default": 0.0,
            "description": "Time to transition to the new value. 0 means instant.",
        }

    return {
        "type": "object",
        "properties": properties,
        "required": required,
    }, required, value_arg_name


# ----------------------------------------------------------------- tier confirm

CONFIRM_PROPERTY_DESCRIPTION = (
    "Confirmation token returned by the previous call to this tool. "
    "Tier 2 and Tier 3 tools require a two-step handshake: the first call "
    "returns a confirmation_token in tier_enforcement; re-call with confirm "
    "set to that token (within 30 seconds) to actually execute. Omit on the "
    "first call."
)


def inject_confirm_if_needed(schema: dict, tier: int) -> None:
    """Tier 2 and Tier 3 tools require a confirmation handshake. The
    dispatcher consumes a `confirm` arg on the second call; without
    declaring it in the schema, AI clients that respect the schema (or
    enforce additionalProperties: false) refuse to send the field. Add
    `confirm` as an optional string property; do not add it to required.
    Tier 1 tools execute immediately and don't need the slot."""
    if tier < 2:
        return
    properties = schema.setdefault("properties", {})
    properties["confirm"] = {
        "type": "string",
        "description": CONFIRM_PROPERTY_DESCRIPTION,
    }


def derive_osc_path(row: CSVRow) -> tuple[str, str | None]:
    """Return (path_or_template, template_form_or_none).

    For layouts without an OSC-path column, derive the path from the
    app's convention table. For the others, parse the OSC path column.
    If the row is part of a numeric-suffix family, return a template
    like '/wfs/input/arrayAtten{array}'.
    """
    csv_file = row.csv_file
    explicit = row.osc_path.strip()
    family = detect_numeric_family(row.variable)
    band = detect_band_placeholder(row.variable)

    if explicit:
        # Strip the trailing placeholders (e.g. " <ID> <value>" or " <ID> <band> <value>").
        path = explicit.split(" ")[0].split("\t")[0].strip()
        # If this row is part of a numeric-suffix family, parameterize the
        # path's trailing digits.
        if family is not None and not band:
            stem, num = family
            # The path likely also has the digit; replace the trailing digit
            # run with {array}.
            path_template = re.sub(r"\d+$", "{array}", path)
            return path, path_template
        return path, None

    # No explicit OSC path - apply convention.
    if csv_file in OSC_PATH_CONVENTION:
        prefix = OSC_PATH_CONVENTION[csv_file]
        return f"{prefix}/{row.variable}", None

    # No path and no convention - leave empty (caller can warn).
    return "", None


# ----------------------------------------------------------------- core

def process_row(row: CSVRow,
                ignore_map: dict[str, str],
                tier_overrides: dict[str, int],
                warnings: list[dict]) -> tuple[dict | None, dict | None, dict | None]:
    """Return (tool_record, nudge_record, ignored_record) - any combination
    may be None."""
    if is_non_parameter_row(row):
        if row.variable.strip():
            # Track skipped rows that had a Variable so they're discoverable.
            return None, None, {
                "variable": row.variable,
                "reason": f"non-parameter UI row (UI={row.ui!r})",
                "csv_file": row.csv_file,
            }
        return None, None, None

    reason = is_ignored(row.variable, ignore_map)
    if reason is not None:
        return None, None, {
            "variable": row.variable,
            "reason": reason,
            "csv_file": row.csv_file,
        }

    csv_namespace = CSV_NAMESPACE.get(row.csv_file, "misc")

    family = detect_numeric_family(row.variable)
    band = detect_band_placeholder(row.variable)

    # Skip the per-instance variants of a numeric-suffix family beyond the
    # first; only the first member becomes a tool, with `array` arg.
    if family is not None and not band:
        stem, num = family
        if num != 1:
            return None, None, None  # silently dedup; the family has one tool

    enum_items, enum_map = parse_enum(row.enum)

    tool_name, nudge_name = derive_tool_name(row, csv_namespace)

    schema, _required, value_arg_name = derive_schema(
        row, tool_name, enum_items, enum_map, family, band,
    )

    description = derive_description(row, csv_namespace,
                                       per_channel=is_per_channel(row))

    # Tier resolution. Precedence: explicit CSV `Tier` column (the primary,
    # authorable source) -> tool_tier_overrides.json (retained fallback) ->
    # keyword/dB-span heuristic. The heuristic warns when it fires so
    # unclassified params are loud, not silent (open-questions-control Q1).
    fam_stem = family[0] if family is not None else None
    tier = parse_tier_cell(row.tier)
    if tier is None:
        if row.tier.strip():
            warnings.append({
                "variable": row.variable,
                "csv_file": row.csv_file,
                "message": f"invalid Tier value {row.tier.strip()!r} "
                           "(expected 1, 2, or 3); ignored",
            })
        tier = lookup_tier_override(row.variable, fam_stem, band, tier_overrides)
    if tier is None:
        tier = heuristic_tier(row.variable, row.label, row.type,
                                row.min, row.max)
        warnings.append({
            "variable": row.variable,
            "csv_file": row.csv_file,
            "message": f"no explicit Tier in CSV; heuristic assigned tier "
                       f"{tier} — add an explicit Tier column value",
        })

    # Phase 8: tier-2/3 schemas declare an optional `confirm` field so
    # AI clients can satisfy the two-step handshake. Tier-1 schemas are
    # untouched. Schema is mutated in place — derive_schema returns
    # before tier is known, so this is the right insertion point.
    inject_confirm_if_needed(schema, tier)

    # OSC path
    osc_path, osc_template = derive_osc_path(row)

    group_key = derive_group_key(row.section, row.variable, csv_namespace)
    domains   = derive_domains(row, csv_namespace)

    # Bake the tier marker into the description text so the model sees
    # it on first read. _meta.tier is also surfaced on each tools/list
    # entry, but most MCP clients don't pass _meta to the model.
    description = append_tier_suffix(description, tier)

    record: dict[str, Any] = {
        "name": tool_name,
        "description": description,
        "parameters": schema,
        "tier": tier,
        "csv_section": row.section,
        "group_key": group_key,
        "supports_relative": row.osc_inc_dec.strip().lower() == "y",
    }
    if domains:
        record["domains"] = domains
    if osc_template is not None:
        record["internal_osc_path_template"] = osc_template
        record["internal_variable_template"] = re.sub(
            r"\d+$", "{array}", row.variable,
        ) if family else row.variable
    else:
        record["internal_osc_path"] = osc_path
        record["internal_variable"] = row.variable.replace(BAND_PLACEHOLDER, "")

    if enum_map is not None:
        record["enum_string_to_int"] = enum_map

    # Tell the C++ loader which JSON key carries the actual value. The
    # default ("value") is dropped to keep the JSON compact; only emit
    # for self-documenting renamings ("name", "mode", "shape", "protocol").
    if value_arg_name != "value":
        record["value_arg_name"] = value_arg_name

    if record["supports_relative"]:
        record["relative_tool_name"] = nudge_name

    # Hover-help missing warning
    if not row.hover.strip():
        warnings.append({
            "variable": row.variable,
            "csv_file": row.csv_file,
            "message": "no hover help text; description may be weak",
        })

    nudge_record = None
    if record["supports_relative"] and nudge_name is not None:
        nudge_desc = (
            "Relative-adjustment variant of "
            f"`{tool_name}`. Sends an `inc` or `dec` step."
        )
        if is_per_channel(row):
            nudge_desc += " " + PER_CHANNEL_HINT
        nudge_desc = append_tier_suffix(nudge_desc, tier)
        nudge_record = {
            "name": nudge_name,
            "description": nudge_desc,
            "parameters": {
                "type": "object",
                "properties": {
                    **{k: v for k, v in schema["properties"].items()
                        if k != "value" and k != "transition_seconds"},
                    "direction": {
                        "type": "string", "enum": ["inc", "dec"],
                        "description": "Direction of the relative step.",
                    },
                    "amount": {
                        "type": "number",
                        "default": 1.0,
                        "description": "Step size.",
                    },
                },
                "required": [k for k in schema["required"] if k != "value"]
                            + ["direction"],
            },
            "tier": tier,
            "internal_osc_path": osc_path,
            "internal_variable": row.variable.replace(BAND_PLACEHOLDER, ""),
            "csv_section": row.section,
            "group_key": group_key,
            "supports_relative": True,
        }
        if domains:
            nudge_record["domains"] = domains

    return record, nudge_record, None


def validate_tools(tools: list[dict]) -> list[str]:
    """Return a list of validation errors. Empty means everything passed."""
    errors: list[str] = []
    seen_names: set[str] = set()
    for t in tools:
        name = t.get("name", "")
        if not name:
            errors.append(f"tool with empty name: {t}")
            continue
        if name in seen_names:
            errors.append(f"duplicate tool name: {name}")
        seen_names.add(name)
        if not t.get("description", "").strip():
            errors.append(f"{name}: empty description")
        if t.get("tier") not in (1, 2, 3):
            errors.append(f"{name}: bad tier {t.get('tier')}")
        if not t.get("group_key", "").strip():
            errors.append(f"{name}: empty group_key")
        # JSON schema round-trip check.
        try:
            json.loads(json.dumps(t["parameters"]))
        except Exception as e:
            errors.append(f"{name}: schema not round-trippable: {e}")
        # Per-channel tools must have channel-id as first required arg.
        for csv_f, (cid, _, _, _) in CHANNEL_ID_RANGE.items():
            internal_var = t.get("internal_variable",
                                  t.get("internal_variable_template", ""))
            csv_ns = CSV_NAMESPACE[csv_f]
            if name.startswith(csv_ns + "."):
                if t["parameters"].get("required", [None])[0] != cid:
                    errors.append(
                        f"{name}: per-channel tool's first required arg is not {cid!r}"
                    )
                break
    return errors


# ----------------------------------------------------------------- I/O

def _normalized_bytes(p: Path) -> bytes:
    """Read a file and normalize line endings to LF, so the hash is stable
    across platforms regardless of git's autocrlf setting."""
    return p.read_bytes().replace(b"\r\n", b"\n").replace(b"\r", b"\n")


def hash_inputs(csv_dir: Path, override_files: list[Path]) -> str:
    h = hashlib.sha256()
    for fname in CSV_FILES_ORDER:
        p = csv_dir / fname
        if p.exists():
            h.update(fname.encode())
            h.update(_normalized_bytes(p))
    for of in override_files:
        if of.exists():
            h.update(of.name.encode())
            h.update(_normalized_bytes(of))
    return h.hexdigest()


def write_json(path: Path, data: dict) -> bool:
    """Write JSON deterministically. Returns True if the file was written,
    False if the on-disk content already matched and the write was skipped.

    Insertion order is preserved (Python 3.7+ dicts are insertion-ordered)
    so the schema's `properties` keep channel_id first, then sub-index,
    then the value arg — the order operators read top-to-bottom. The tools
    list itself is pre-sorted by name in main(); ignored / warnings /
    groups are sorted at construction. Determinism is guaranteed by
    construction, not by sort_keys=True (which would scramble the
    semantically-meaningful ordering of `properties`).
    """
    text = json.dumps(data, indent=2, ensure_ascii=False) + "\n"
    new_bytes = text.encode("utf-8")
    path.parent.mkdir(parents=True, exist_ok=True)
    # Skip the write if the on-disk content already matches. Compare with line
    # endings normalized on both sides, so a CRLF-on-disk checkout (Windows
    # core.autocrlf=true) doesn't trigger a rewrite when the semantic content
    # is identical. Avoids touching the file's mtime and keeps the working
    # tree clean across rebuilds when the input CSVs haven't changed.
    if path.exists():
        try:
            existing = path.read_bytes().replace(b"\r\n", b"\n").replace(b"\r", b"\n")
            if existing == new_bytes:
                return False
        except OSError:
            pass
    with path.open("wb") as f:
        f.write(new_bytes)
    return True


def main(argv: list[str] | None = None) -> int:
    if not _CONFIGURED:
        print("error: generator core is unconfigured - call "
              "configure(<app config module>) first (WFS-DIY installs "
              "tools/mcp/wfs_codegen_config.py via the "
              "tools/generate_mcp_tools.py wrapper)", file=sys.stderr)
        return 2

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--csv-dir", default=DEFAULT_CSV_DIR,
                        help="Directory containing the source CSV files.")
    parser.add_argument("--overrides-tier",
                        default=DEFAULT_OVERRIDES_TIER)
    parser.add_argument("--overrides-ignore",
                        default=DEFAULT_OVERRIDES_IGNORE)
    parser.add_argument("--output",
                        default=DEFAULT_OUTPUT)
    parser.add_argument("--groups-output",
                        default=DEFAULT_GROUPS_OUTPUT)
    parser.add_argument("--force", action="store_true",
                        help="Force regeneration even if input hash matches.")
    args = parser.parse_args(argv)

    csv_dir = Path(args.csv_dir)
    out_path = Path(args.output)
    groups_path = Path(args.groups_output)
    tier_path = Path(args.overrides_tier)
    ignore_path = Path(args.overrides_ignore)

    if not csv_dir.is_dir():
        print(f"error: csv-dir not found: {csv_dir}", file=sys.stderr)
        return 2

    # Fast-path: hash inputs and skip if matching the existing output.
    input_hash = hash_inputs(csv_dir, [tier_path, ignore_path])
    if not args.force and out_path.exists():
        try:
            existing = json.loads(out_path.read_text(encoding="utf-8"))
            if existing.get("input_hash") == input_hash:
                print(f"up-to-date: {out_path} (hash {input_hash[:12]}...)")
                return 0
        except Exception:
            pass

    tier_overrides = load_overrides_tier(tier_path)
    ignore_map = load_overrides_ignore(ignore_path)

    # Pre-pass: identify which numeric-suffix stems are real families
    # (≥ 2 sister variables sharing the same stem). Without this, lone
    # parameters whose names end in digits — such as `reverbRT60` — are
    # mistakenly treated as the 60th member of a 60-element family and
    # silently skipped by process_row's family dedup.
    global _REAL_FAMILY_STEMS
    _REAL_FAMILY_STEMS = compute_real_family_stems(csv_dir)

    tools: list[dict] = []
    nudge_tools: list[dict] = []
    ignored: list[dict] = []
    warnings: list[dict] = []

    consumed_csvs: list[str] = []
    seen_variables: set[tuple[str, str]] = set()
    for fname in CSV_FILES_ORDER:
        p = csv_dir / fname
        if not p.exists():
            warnings.append({"csv_file": fname,
                             "message": "file missing - skipped"})
            continue
        consumed_csvs.append(fname)
        for row in read_csv(p):
            # Skip non-parameter UI documentation rows BEFORE deduping, so
            # they don't claim a Variable slot that a real parameter row
            # later in the CSV needs (the EQ band-toggle and EQ display rows
            # share their Variable with the actual shape-selector row).
            if is_non_parameter_row(row):
                if row.variable.strip():
                    ignored.append({
                        "variable": row.variable,
                        "reason": f"non-parameter UI row (UI={row.ui!r})",
                        "csv_file": row.csv_file,
                    })
                continue
            # Deduplicate rows that share the same Variable within a single
            # CSV. The exhaustive ADM-OSC and audio-patch tables have many
            # rows with the same paramId differing only by Label; the MCP
            # tool surface needs one tool per paramId, with the per-instance
            # selection (mapping, axis, channel) handled by the C++
            # dispatcher at call time.
            if row.variable.strip():
                key = (fname, row.variable.strip())
                if key in seen_variables:
                    continue
                seen_variables.add(key)
            tool, nudge, ig = process_row(row, ignore_map, tier_overrides,
                                            warnings)
            if tool is not None:
                tools.append(tool)
            if nudge is not None:
                nudge_tools.append(nudge)
            if ig is not None:
                ignored.append(ig)

    # Sort everything for determinism.
    tools.sort(key=lambda t: t["name"])
    nudge_tools.sort(key=lambda t: t["name"])
    ignored.sort(key=lambda i: (i["csv_file"], i["variable"]))
    warnings.sort(key=lambda w: (w.get("csv_file", ""), w.get("variable", "")))

    # Validate.
    errors = validate_tools(tools)
    if errors:
        print("validation errors:", file=sys.stderr)
        for e in errors:
            print(f"  {e}", file=sys.stderr)
        return 3

    # Build the group-key lookup.
    groups: dict[str, list[str]] = defaultdict(list)
    for t in tools:
        groups[t["group_key"]].append(t["name"])
    for k in groups:
        groups[k].sort()
    groups_sorted = dict(sorted(groups.items()))

    payload = {
        "schema_version": SCHEMA_VERSION,
        "input_hash": input_hash,
        "source_csvs": consumed_csvs,
        "tools": tools,
        "nudge_tools": nudge_tools,
        "ignored_parameters": ignored,
        "warnings": warnings,
    }
    tools_written = write_json(out_path, payload)

    groups_payload = {
        "schema_version": SCHEMA_VERSION,
        "input_hash": input_hash,
        "groups": groups_sorted,
    }
    groups_written = write_json(groups_path, groups_payload)

    tools_verb = "wrote" if tools_written else "unchanged"
    groups_verb = "wrote" if groups_written else "unchanged"
    print(f"{tools_verb} {out_path} - {len(tools)} tools, "
          f"{len(nudge_tools)} nudge variants, {len(ignored)} ignored, "
          f"{len(warnings)} warnings")
    print(f"{groups_verb} {groups_path} - {len(groups_sorted)} groups")
    return 0


if __name__ == "__main__":
    sys.exit(main())
