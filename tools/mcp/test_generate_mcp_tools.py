"""Tests for tools/generate_mcp_tools.py.

Run from the repo root with:

    python -m pytest tools/mcp/

The golden tests verify the structural shape of representative output
records (the patterns from `Documentation/MCP/specs/GENERATION_SCRIPT_SPEC.md`
§ Appendix). Description text is checked for keywords rather than full
byte-equality, since the generator's description-builder follows the spec's
rule of "hover help + unit + transition note + per-channel hint" rather than
the spec's hand-crafted appendix prose.
"""

from __future__ import annotations

import io
import json
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parents[2]
GENERATOR = REPO_ROOT / "tools" / "generate_mcp_tools.py"

# Make the generator importable for unit testing.
sys.path.insert(0, str(GENERATOR.parent))
import generate_mcp_tools as g  # type: ignore  # noqa: E402


# ----------------------------------------------------------------- helpers

def make_row(csv_file: str, **kwargs) -> g.CSVRow:
    """Construct a CSVRow with default empty fields, overriding with kwargs."""
    return g.CSVRow(csv_file=csv_file, **kwargs)


def run_process(row: g.CSVRow, *,
                 ignore_map: dict[str, str] | None = None,
                 tier_overrides: dict[str, int] | None = None) -> dict:
    """Run process_row and return the tool record (or fail)."""
    warnings: list[dict] = []
    tool, _nudge, ignored = g.process_row(
        row,
        ignore_map=ignore_map or {},
        tier_overrides=tier_overrides or {},
        warnings=warnings,
    )
    if ignored:
        pytest.fail(f"unexpectedly ignored: {ignored}")
    if tool is None:
        pytest.fail("process_row returned None tool")
    return tool


# ----------------------------------------------------------------- golden A

def test_goldenA_input_attenuation():
    """Plain float per-channel parameter."""
    row = make_row(
        "WFS-UI_input.csv",
        section="Attenuation",
        label="Attenuation",
        variable="inputAttenuation",
        ui="H Slider",
        type="FLOAT",
        min="-92.0",
        max="0.0",
        default="0.0",
        formula="20*log10(...)",
        unit="dB",
        osc_path="/wfs/input/attenuation",
        osc_inc_dec="y",
        hover="Input Channel Attenuation.",
    )
    tool = run_process(
        row,
        tier_overrides={"inputAttenuation": 2},
    )
    assert tool["name"] == "input.set_attenuation"
    assert tool["tier"] == 2
    assert tool["internal_osc_path"] == "/wfs/input/attenuation"
    assert tool["internal_variable"] == "inputAttenuation"
    assert tool["csv_section"] == "Attenuation"
    assert tool["group_key"] == "input_attenuation"
    assert tool["supports_relative"] is True
    assert tool["relative_tool_name"] == "input.nudge_attenuation"
    props = tool["parameters"]["properties"]
    assert props["input_id"]["minimum"] == 1
    assert props["input_id"]["maximum"] == 64
    assert props["value"]["type"] == "number"
    assert props["value"]["minimum"] == -92.0
    assert props["value"]["maximum"] == 0.0
    assert tool["parameters"]["required"] == ["input_id", "value"]


# ----------------------------------------------------------------- golden B

def test_goldenB_output_eq_freq():
    """Per-channel + per-band parameter using <band> placeholder."""
    row = make_row(
        "WFS-UI_output.csv",
        section="EQ",
        label="EQ Frequency",
        variable="outputEQfreq<band>",
        ui="H slider",
        type="INT",
        min="20",
        max="20000",
        default="80 / 250 / 1000 / 4000 / 8000 / 12000",
        formula="20*pow(10,3*x)",
        unit="Hz",
        notes="For each of 6 EQ bands.",
        osc_path="/wfs/output/Eqfreq <ID> <band> <value>",
    )
    tool = run_process(row)
    assert tool["name"] == "output.eq.set_frequency"
    assert tool["tier"] == 1
    assert tool["internal_osc_path"] == "/wfs/output/Eqfreq"
    # The <band> placeholder is stripped from internal_variable so the C++
    # dispatcher sees the prefix only.
    assert tool["internal_variable"] == "outputEQfreq"
    assert tool["csv_section"] == "EQ"
    assert tool["group_key"] == "output_eq"
    assert tool["supports_relative"] is False
    props = tool["parameters"]["properties"]
    assert "output_id" in props
    assert "band" in props
    assert props["band"]["minimum"] == 1
    assert props["band"]["maximum"] == 6
    assert props["value"]["type"] == "integer"
    assert props["value"]["minimum"] == 20
    assert props["value"]["maximum"] == 20000


# ----------------------------------------------------------------- golden C

def test_goldenC_input_array_attenuation():
    """Sub-indexed array-family parameter (numeric suffix in Variable)."""
    row = make_row(
        "WFS-UI_input.csv",
        section="Array Attenuation",
        label="Array 1 Attenuation",
        variable="inputArrayAtten1",
        ui="Dial",
        type="FLOAT",
        min="-60.0",
        max="0.0",
        default="0.0",
        unit="dB",
        notes="Dimmed if no outputs assigned to Array 1",
        osc_path="/wfs/input/arrayAtten1 <ID> <value>",
    )
    tool = run_process(
        row,
        tier_overrides={"inputArrayAtten*": 2},
    )
    assert tool["name"] == "input.set_array_attenuation"
    assert tool["tier"] == 2
    # The path becomes a template form because the family parameterizes the
    # trailing digit.
    assert tool["internal_osc_path_template"] == "/wfs/input/arrayAtten{array}"
    assert tool["internal_variable_template"] == "inputArrayAtten{array}"
    assert tool["csv_section"] == "Array Attenuation"
    assert tool["group_key"] == "input_array_attenuation"
    props = tool["parameters"]["properties"]
    assert "input_id" in props
    assert "array" in props
    assert props["array"]["minimum"] == 1
    assert props["array"]["maximum"] == 10


def test_goldenC_dedup_array_family():
    """Only the first member of a numeric-suffix family produces a tool."""
    row1 = make_row(
        "WFS-UI_input.csv",
        section="Array Attenuation",
        label="Array 1 Attenuation",
        variable="inputArrayAtten1",
        type="FLOAT", min="-60.0", max="0.0",
        osc_path="/wfs/input/arrayAtten1 <ID> <value>",
    )
    row5 = make_row(
        "WFS-UI_input.csv",
        section="Array Attenuation",
        label="Array 5 Attenuation",
        variable="inputArrayAtten5",
        type="FLOAT", min="-60.0", max="0.0",
        osc_path="/wfs/input/arrayAtten5 <ID> <value>",
    )
    warnings: list[dict] = []
    t1, _, _ = g.process_row(row1, {}, {}, warnings)
    t5, _, _ = g.process_row(row5, {}, {}, warnings)
    assert t1 is not None
    assert t5 is None  # deduplicated; only `inputArrayAtten1` becomes a tool


# ----------------------------------------------------------------- golden D

def test_goldenD_output_eq_shape_enum():
    """Enum-typed parameter with non-sequential stored IDs."""
    row = make_row(
        "WFS-UI_output.csv",
        section="EQ",
        label="EQ Shape",
        variable="outputEQshape<band>",
        ui="Drop down menu",
        type="INT",
        min="1",
        max="7",
        enum="Low Cut (1) ; Low Shelf (2) ; Peak/Notch (3) ; Band Pass (4) "
              "; High Shelf (5) ; High Cut (6) ; All Pass (7)",
        osc_path="/wfs/output/EQshape <ID> <band> <value>",
    )
    tool = run_process(row)
    assert tool["name"] == "output.eq.set_shape"
    assert tool["tier"] == 1
    enum_map = tool["enum_string_to_int"]
    # Non-sequential mapping per the spec.
    assert enum_map["BandPass"] == 4
    assert enum_map["HighShelf"] == 5
    assert enum_map["HighCut"] == 6
    assert enum_map["AllPass"] == 7
    props = tool["parameters"]["properties"]
    assert "shape" in props
    assert props["shape"]["type"] == "string"
    assert "BandPass" in props["shape"]["enum"]
    assert "AllPass" in props["shape"]["enum"]


# ----------------------------------------------------------------- golden E

def test_goldenE_network_protocol_12col_layout():
    """12-column-layout row (no Formula, no OSC path; path derived by convention)."""
    row = make_row(
        "WFS-UI_network.csv",
        section="Network",
        label="Target/Server Protocol",
        variable="networkTSProtocol",
        ui="drop down menu",
        type="INT",
        min="0",
        max="7",
        default="0",
        enum="DISABLED ; OSC ; Remote ; ADM-OSC ; QLab",
        notes="For each target/server.",
        hover="Select the Protocol: DISABLED, OSC, REMOTE, or ADM-OSC.",
    )
    tool = run_process(row)
    # Variable is `networkTSProtocol` -> stripped to `TSProtocol` -> snake-cased.
    # Section "Network" -> namespace "network" (CSV namespace).
    # The exact tool-name shape depends on stripping rules; the contract is:
    # - starts with "network." (CSV namespace),
    # - includes "protocol" somewhere in the action segment.
    assert tool["name"].startswith("network.")
    assert "protocol" in tool["name"]
    # 12-column layout means OSC path is derived from convention.
    assert tool["internal_osc_path"].startswith("/wfs/network/")
    assert tool["csv_section"] == "Network"


# ----------------------------------------------------------------- override / ignore

def test_override_takes_precedence_over_heuristic():
    row = make_row(
        "WFS-UI_input.csv",
        section="Position",
        label="Position X",
        variable="inputPositionX",
        type="FLOAT", min="-50.0", max="50.0",
        osc_path="/wfs/input/positionX",
        hover="Object Position in Width.",
    )
    # Heuristic alone would land on tier 1 for this row (narrow range, no
    # dangerous keywords). Override flips it to 3.
    tool = run_process(row, tier_overrides={"inputPositionX": 3})
    assert tool["tier"] == 3


def test_ignore_list_drops_row():
    row = make_row(
        "WFS-UI_input.csv",
        section="Gradient Map",
        label="Shape Locked",
        variable="gmShapeLocked",
        type="INT", min="0", max="1",
    )
    warnings: list[dict] = []
    tool, _, ignored = g.process_row(
        row,
        ignore_map={"gmShapeLocked": "UI-state only"},
        tier_overrides={},
        warnings=warnings,
    )
    assert tool is None
    assert ignored is not None
    assert ignored["variable"] == "gmShapeLocked"


# ----------------------------------------------------------------- enum parsing

def test_parse_enum_with_explicit_ids():
    items, mapping = g.parse_enum(
        "Low Cut (1) ; Low Shelf (2) ; Peak/Notch (3) ; Band Pass (4)"
    )
    assert items == ["LowCut", "LowShelf", "PeakNotch", "BandPass"]
    assert mapping == {"LowCut": 1, "LowShelf": 2, "PeakNotch": 3, "BandPass": 4}


def test_parse_enum_without_explicit_ids():
    items, mapping = g.parse_enum("OFF ; ON")
    assert items == ["OFF", "ON"]
    assert mapping is None


def test_parse_enum_empty():
    items, mapping = g.parse_enum("")
    assert items == []
    assert mapping is None


# ----------------------------------------------------------------- group_key

def test_group_key_strips_coordinate_suffix():
    assert g.derive_group_key("Position", "inputPositionX", "input") \
        == "input_position"
    assert g.derive_group_key("Position (Cylindrical)", "inputPositionR",
                               "input") == "input_position"
    assert g.derive_group_key("Position (Spherical)", "inputPositionPhi",
                               "input") == "input_position"


# ----------------------------------------------------------------- determinism / fast-path

def test_integration_runs_against_live_csvs(tmp_path):
    """Run the generator against the live CSVs and verify a sane output shape."""
    out_path = tmp_path / "generated_tools.json"
    groups_path = tmp_path / "generated_groups.json"
    rc = g.main([
        "--csv-dir", str(REPO_ROOT / "Documentation"),
        "--overrides-tier", str(REPO_ROOT / "tools/mcp/tool_tier_overrides.json"),
        "--overrides-ignore", str(REPO_ROOT / "tools/mcp/tool_generation_ignores.json"),
        "--output", str(out_path),
        "--groups-output", str(groups_path),
        "--force",
    ])
    assert rc == 0
    data = json.loads(out_path.read_text(encoding="utf-8"))
    tools = data["tools"]
    assert 200 <= len(tools) <= 800, f"unexpected tool count: {len(tools)}"
    # Tools sorted by name.
    names = [t["name"] for t in tools]
    assert names == sorted(names)
    # Every tool has a tier in {1,2,3} and a non-empty group_key.
    for t in tools:
        assert t["tier"] in (1, 2, 3), t["name"]
        assert t["group_key"], t["name"]
        assert t["description"], t["name"]


def test_idempotency_fast_path(tmp_path):
    """Second run with the same inputs should be a fast-path no-op."""
    out_path = tmp_path / "generated_tools.json"
    groups_path = tmp_path / "generated_groups.json"
    common = [
        "--csv-dir", str(REPO_ROOT / "Documentation"),
        "--overrides-tier", str(REPO_ROOT / "tools/mcp/tool_tier_overrides.json"),
        "--overrides-ignore", str(REPO_ROOT / "tools/mcp/tool_generation_ignores.json"),
        "--output", str(out_path),
        "--groups-output", str(groups_path),
    ]
    # First run with --force to land an output.
    assert g.main(common + ["--force"]) == 0
    bytes1 = out_path.read_bytes()
    # Second run with no --force should detect the matching hash and skip.
    captured = io.StringIO()
    rc = g.main(common)
    assert rc == 0
    bytes2 = out_path.read_bytes()
    # Bytes should be identical (the fast-path doesn't rewrite).
    assert bytes1 == bytes2
