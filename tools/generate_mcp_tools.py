#!/usr/bin/env python3
"""Generate the MCP tool surface from WFS-DIY's WFS-UI_*.csv source CSVs.

Run from the repo root with default args. See tools/mcp/README.md for usage.
Spec: Documentation/MCP/specs/GENERATION_SCRIPT_SPEC.md.
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

# CSV file -> tool namespace prefix used in tool names and group_keys.
CSV_NAMESPACE = {
    "WFS-UI_input.csv":      "input",
    "WFS-UI_output.csv":     "output",
    "WFS-UI_reverb.csv":     "reverb",
    "WFS-UI_clusters.csv":   "cluster",
    "WFS-UI_network.csv":    "network",
    "WFS-UI_config.csv":     "system",
    "WFS-UI_audioPatch.csv": "audio",
}

# OSC-path convention for the 12-column CSVs that don't carry an explicit path.
OSC_PATH_CONVENTION = {
    "WFS-UI_config.csv":  "/wfs/config",
    "WFS-UI_network.csv": "/wfs/network",
}

# Variable-prefix stripping per CSV (longest match wins).
VARIABLE_PREFIXES = {
    "WFS-UI_input.csv":   ["input"],
    "WFS-UI_output.csv":  ["output"],
    "WFS-UI_reverb.csv":  ["reverb"],
    "WFS-UI_clusters.csv": ["cluster"],
    "WFS-UI_network.csv":  ["network", "tracking", "admCart", "admPolar",
                            "admOsc", "findDevice"],
    "WFS-UI_config.csv":   [],  # variable names are PascalCase here
    "WFS-UI_audioPatch.csv": ["sampler"],
}

# Common abbreviations expanded for human-readable tool names.
# Order matters: longer abbreviations first to avoid sub-matches.
ABBREVIATIONS = [
    ("HFdamping",   "hf_damping"),
    ("HFshelf",     "hf_shelf"),
    ("LFOactive",   "lfo_active"),
    ("LFOperiod",   "lfo_period"),
    ("LFOphase",    "lfo_phase"),
    ("LFOshape",    "lfo_shape"),
    ("LFOrate",     "lfo_rate"),
    ("LFOamplitude", "lfo_amplitude"),
    ("LFOgyrophone", "lfo_gyrophone"),
    ("LSenable",    "live_source_enable"),
    ("LSactive",    "live_source_active"),
    ("LSradius",    "live_source_radius"),
    ("LSshape",     "live_source_shape"),
    ("LSattenuation", "live_source_attenuation"),
    ("LSpeak",      "live_source_peak"),
    ("LSslow",      "live_source_slow"),
    ("LSatten",     "live_source_atten"),
    ("FRactive",    "floor_reflections_active"),
    ("FRattenuation", "floor_reflections_attenuation"),
    ("FRlowCut",    "floor_reflections_low_cut"),
    ("FRhighShelf", "floor_reflections_high_shelf"),
    ("FRdiffusion", "floor_reflections_diffusion"),
    ("FRenable",    "floor_reflections_enable"),
    ("EQfreq",      "eq_frequency"),
    ("EQshape",     "eq_shape"),
    ("EQgain",      "eq_gain"),
    ("EQq",         "eq_q"),
    ("EQslope",     "eq_slope"),
    ("EQenable",    "eq_enable"),
    ("EQenabled",   "eq_enabled"),
    ("eqShape",     "eq_shape"),
    ("eqFrequency", "eq_frequency"),
    ("eqGain",      "eq_gain"),
    ("eqQ",         "eq_q"),
    ("eqSlope",     "eq_slope"),
    ("EQ",          "eq"),
    ("LFO",         "lfo"),
    ("LS",          "live_source"),
    ("FR",          "floor_reflections"),
    ("HF",          "hf"),
    ("Hparallax",   "h_parallax"),
    ("Vparallax",   "v_parallax"),
    ("OSC",         "osc"),
    ("admCart",     "adm_cart"),
    ("admPolar",    "adm_polar"),
    ("admOsc",      "adm_osc"),
    ("RT60",        "rt60"),
    ("SDN",         "sdn"),
    ("FDN",         "fdn"),
    ("IR",          "ir"),
    ("AutomOtion",  "automotion"),
    ("MQTT",        "mqtt"),
    ("PSN",         "psn"),
    ("RTTrP",       "rttrp"),
    ("UDP",         "udp"),
    ("TCP",         "tcp"),
    ("UI",          "ui"),
    ("ID",          "id"),
    ("XY",          "xy"),
    ("XYZ",         "xyz"),
    ("3D",          "3d"),
    ("3d",          "3d"),
    ("Rsph",        "r_spherical"),
    ("Phi",         "phi"),
    ("Theta",       "theta"),
    ("DSP",         "dsp"),
]

# Common word fragments to expand once the snake_case form is built.
WORD_EXPANSIONS = {
    "atten":  "attenuation",
    "freq":   "frequency",
    "param":  "parameter",
    "config": "configuration",
}

# Variable-suffix → short label substitutions for nudge-tool naming.
NUDGE_VERB = "nudge"
SET_VERB = "set"

# Tier heuristics (defaults; overrides take precedence).
TIER_KEYWORDS_3 = ["delete", "clear", "remove", "reset", "channels",
                   "sampleRate", "runDSP", "reconfigure"]
TIER_KEYWORDS_2 = ["master", "solo", "muteAll", "testTone", "store", "load",
                   "import", "export", "snapshot"]


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


def detect_band_placeholder(variable: str) -> bool:
    return "<band>" in variable


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


def lookup_tier_override(variable: str, family_stem: str | None,
                          band_placeholder: bool,
                          overrides: dict[str, int]) -> int | None:
    """Apply override-key lookup: literal, then `<band>` placeholder, then `*`
    wildcard for numeric-suffix families."""
    if variable in overrides:
        return overrides[variable]
    if band_placeholder:
        # The Variable cell already contains `<band>`; literal lookup above
        # already handles it. Nothing extra here.
        pass
    if family_stem is not None:
        wildcard_key = family_stem + "*"
        if wildcard_key in overrides:
            return overrides[wildcard_key]
    return None


def heuristic_tier(variable: str, label: str, type_: str,
                    min_v: str, max_v: str) -> int:
    """Default tier classification when no override applies."""
    name = variable + " " + label
    name_lower = name.lower()
    for kw in TIER_KEYWORDS_3:
        if kw.lower() in name_lower:
            return 3
    for kw in TIER_KEYWORDS_2:
        if kw.lower() in name_lower:
            return 2
    # Wide attenuation/dB ranges → tier 2 (sudden loud output risk).
    try:
        if type_.upper() in ("FLOAT", "INT") and min_v and max_v:
            span = float(max_v) - float(min_v)
            if span >= 40 and ("attenuation" in name_lower or
                                "level" in name_lower or
                                "atten" in name_lower):
                return 2
    except ValueError:
        pass
    return 1


# ----------------------------------------------------------------- group_key

# Coordinate-system suffixes to strip from Section before computing group_key.
_COORD_SUFFIX_RE = re.compile(r"\s*\((Cylindrical|Spherical|Cartesian)\)\s*$")


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

# UI-control vocabulary that disqualifies a row from becoming a tool.
NON_PARAMETER_UI = {
    "Display label", "Read-only label", "Read-only labels (X / Y / Z)",
    "Display label (X / Y / Z)", "Visual indicator (live)",
    "Visual indicator", "Tab selector", "Window", "Internal state",
    "Internal state (atomic)", "Display label", "Mouse gesture",
    "Mouse / touch gesture", "Keyboard", "Scrollbar (16 px)",
    "2D grid (scrollable)", "interactive display", "6x6 cell grid",
    "Generator (DSP)", "meter", "SVG drawable", "Help card button",
    "(internal)", "interactive display", "(no editor)", "Read-only progress dial",
    "Help button", "Mouse / touch", "Mouse",
    "Band toggle",  # UI affordance for the EQ-band on/off shared paramId
    "interactive display",
    "Read-only progress dial",
    "Mouse / touch", "Mouse",
}

# Variables that are purely display / non-persisted UI affordances.
NON_PARAMETER_VARIABLES_PREFIXES = (
    "JoystickXY", "JoystickZ",
)


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

    # Strip <band> placeholder for naming purposes
    name_var = variable.replace("<band>", "")
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
        parts.append(
            "Use `session_get_state()` first if unsure which channels exist."
        )
    return ". ".join(p.rstrip(".") for p in parts) + "."


# Per-CSV channel-id range. Used to set the maximum channel-id integer.
CHANNEL_ID_RANGE = {
    "WFS-UI_input.csv":   ("input_id", 1, 64),
    "WFS-UI_output.csv":  ("output_id", 1, 64),
    "WFS-UI_reverb.csv":  ("reverb_id", 1, 16),
    "WFS-UI_clusters.csv": ("cluster_id", 1, 10),
}


def is_per_channel(row: CSVRow) -> bool:
    return row.csv_file in CHANNEL_ID_RANGE


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
        cid_name, cid_min, cid_max = CHANNEL_ID_RANGE[row.csv_file]
        properties[cid_name] = {
            "type": "integer",
            "minimum": cid_min,
            "maximum": cid_max,
            "description": f"{row.csv_file.replace('WFS-UI_', '').replace('.csv', '').capitalize()} channel number (1-based).",
        }
        required.append(cid_name)
    elif row.csv_file == "WFS-UI_audioPatch.csv" and "Cell" in row.label:
        # Sampler cell index in the 6x6 grid.
        properties["cell_id"] = {
            "type": "integer", "minimum": 0, "maximum": 35,
            "description": "Sampler cell index (0-35, 6x6 grid).",
        }
        required.append("cell_id")

    # Sub-index argument if applicable.
    if band:
        properties["band"] = {
            "type": "integer", "minimum": 1, "maximum": 6,
            "description": "EQ band number (1-6).",
        }
        required.append("band")
    elif family is not None and row.csv_file == "WFS-UI_input.csv" \
            and family[0].lower().endswith("atten"):
        properties["array"] = {
            "type": "integer", "minimum": 1, "maximum": 10,
            "description": "Output array number (1-10).",
        }
        required.append("array")
    elif family is not None:
        # Generic numeric-suffix family - expose as `index`.
        properties["index"] = {
            "type": "integer", "minimum": 1,
            "description": "Item index (1-based).",
        }
        required.append("index")

    # Value argument.
    type_str = row.type.strip().upper()
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
        properties["value"] = val_arg
        required.append("value")
    elif type_str.startswith("STRING"):
        val_arg = {
            "type": "string",
            "description": (row.label.strip() or "Value") + ".",
        }
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
        properties["value"] = val_arg
        required.append("value")
    else:
        # Catch-all: the row may not be a settable parameter (button, etc.).
        val_arg = {"type": "string", "description": "Value."}
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

    For the 12-column layouts, derive the path from convention. For the
    others, parse the OSC path column. If the row is part of a numeric-suffix
    family, return a template like '/wfs/input/arrayAtten{array}'.
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

    # Tier
    fam_stem = family[0] if family is not None else None
    tier = lookup_tier_override(row.variable, fam_stem, band, tier_overrides)
    if tier is None:
        tier = heuristic_tier(row.variable, row.label, row.type,
                                row.min, row.max)

    # Phase 8: tier-2/3 schemas declare an optional `confirm` field so
    # AI clients can satisfy the two-step handshake. Tier-1 schemas are
    # untouched. Schema is mutated in place — derive_schema returns
    # before tier is known, so this is the right insertion point.
    inject_confirm_if_needed(schema, tier)

    # OSC path
    osc_path, osc_template = derive_osc_path(row)

    group_key = derive_group_key(row.section, row.variable, csv_namespace)

    record: dict[str, Any] = {
        "name": tool_name,
        "description": description,
        "parameters": schema,
        "tier": tier,
        "csv_section": row.section,
        "group_key": group_key,
        "supports_relative": row.osc_inc_dec.strip().lower() == "y",
    }
    if osc_template is not None:
        record["internal_osc_path_template"] = osc_template
        record["internal_variable_template"] = re.sub(
            r"\d+$", "{array}", row.variable,
        ) if family else row.variable
    else:
        record["internal_osc_path"] = osc_path
        record["internal_variable"] = row.variable.replace("<band>", "")

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
            nudge_desc += (
                " Use `session_get_state()` first if unsure which channels exist."
            )
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
            "internal_variable": row.variable.replace("<band>", ""),
            "csv_section": row.section,
            "group_key": group_key,
            "supports_relative": True,
        }

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
        for csv_f, (cid, _, _) in CHANNEL_ID_RANGE.items():
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

CSV_FILES_ORDER = [
    "WFS-UI_config.csv",
    "WFS-UI_input.csv",
    "WFS-UI_output.csv",
    "WFS-UI_reverb.csv",
    "WFS-UI_clusters.csv",
    "WFS-UI_network.csv",
    "WFS-UI_audioPatch.csv",
]


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


def write_json(path: Path, data: dict) -> None:
    """Write JSON deterministically.

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
                return
        except OSError:
            pass
    with path.open("wb") as f:
        f.write(new_bytes)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--csv-dir", default="Documentation",
                        help="Directory containing the WFS-UI_*.csv files.")
    parser.add_argument("--overrides-tier",
                        default="tools/mcp/tool_tier_overrides.json")
    parser.add_argument("--overrides-ignore",
                        default="tools/mcp/tool_generation_ignores.json")
    parser.add_argument("--output",
                        default="Source/Network/MCP/generated_tools.json")
    parser.add_argument("--groups-output",
                        default="Source/Network/MCP/generated_groups.json")
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
    _stems_seen: dict[str, set[int]] = defaultdict(set)
    for fname in CSV_FILES_ORDER:
        p = csv_dir / fname
        if not p.exists():
            continue
        for row in read_csv(p):
            if not row.variable.strip():
                continue
            m = _NUMERIC_FAMILY_RE.match(row.variable)
            if m:
                _stems_seen[m.group(1)].add(int(m.group(2)))
    _REAL_FAMILY_STEMS = {s for s, nums in _stems_seen.items() if len(nums) >= 2}

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
    write_json(out_path, payload)

    groups_payload = {
        "schema_version": SCHEMA_VERSION,
        "input_hash": input_hash,
        "groups": groups_sorted,
    }
    write_json(groups_path, groups_payload)

    print(f"wrote {out_path} - {len(tools)} tools, "
          f"{len(nudge_tools)} nudge variants, {len(ignored)} ignored, "
          f"{len(warnings)} warnings")
    print(f"wrote {groups_path} - {len(groups_sorted)} groups")
    return 0


if __name__ == "__main__":
    sys.exit(main())
