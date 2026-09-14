"""MCP transcript replay driver.

Two run segments against temp copies of the golden fixture:

Segment A (WFS_MCP_AI_ENABLED=1):
  1. initialize                 -> normalized envelope
  2. tools/list                 -> census: count >= 350 (hard assert),
                                   tier ordering contract (tier DESC,
                                   name ASC — hard assert), per-tier counts
  3. tier-1 write + read-back   (input_position_set_x / wfs_get_parameter)
  4. tier-2 confirm round-trip  (input_set_attenuation: awaiting_confirmation
                                   envelope with normalized token, then the
                                   confirmed execution)
  5. tier-3 call                (system_i_o_set_input_channels ->
                                   safety_gate_closed envelope; the gate is
                                   UI-only by design, no automation hook)
  6. 16-write batch             (wfs_set_parameter_batch, tier-2 confirm)
  7. mcp_undo_last_ai_change    -> the WHOLE batch reverts as ONE step
                                   (hard assert via read-back)
  8. mcp_redo_last_undone_ai_change -> batch re-applies (hard assert)
  9. mcp_get_ai_change_history compact (timestamps normalized)
 10. array attenuation          (input_set_array_attenuation, the one tool the
                                   generator writes as a template for the
                                   inputArrayAtten1..10 family: the `array`
                                   argument picks the member; neighbours stay
                                   put; wfs_set/get_parameter and the batch
                                   reach every member; out-of-range index /
                                   value refused; describe lists all ten with
                                   their tool_args; undo reverts the batch,
                                   then the single write)

Segment B (env var absent): one tier-1 call -> ai_disabled envelope.

The normalized transcript is compared against a committed golden
(--update regenerates it). Hard asserts fail the run even in --update mode.

Usage:
  python mcp_replay.py [--exe path] [--update] [--keep-temp]

Exit codes: 0 pass, 1 mismatch, 2 usage, 3 app failed to start.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import sys
from pathlib import Path

import common

GOLDEN = common.GOLDENS_DIR / "mcp_replay.json"

# 16 writes: X and Y for inputs 1..8, all binary-exact values.
BATCH_WRITES = (
    [{"variable": "inputPositionX", "channel_id": i, "value": i * 0.25}
     for i in range(1, 9)]
    + [{"variable": "inputPositionY", "channel_id": i, "value": i * 0.5}
       for i in range(1, 9)]
)

READS = [{"variable": "inputPositionX", "channel_id": i}
         for i in range(1, 9)] + \
        [{"variable": "inputPositionY", "channel_id": i}
         for i in range(1, 9)]


def read_positions(app: common.App):
    payload = common.tool_payload(app.tool("wfs_get_parameters",
                                           {"reads": READS}))
    assert isinstance(payload, dict), f"batch read failed: {payload}"
    return [(r["variable"], r["channel_id"], round(float(r["value"]), 6))
            for r in payload["results"]]


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--exe", default=None)
    p.add_argument("--update", action="store_true")
    p.add_argument("--keep-temp", action="store_true")
    args = p.parse_args()

    exe = common.find_exe(args.exe)
    work_root = Path(os.environ.get("TEMP", ".")) / "wfs-control-replay" \
        / "mcp_replay"

    transcript: list[dict] = []
    hard_failures: list[str] = []

    def record(step: str, envelope: dict) -> dict:
        norm = common.normalize_envelope_text(envelope)
        # JSON-RPC ids are client-side bookkeeping; the id counter's start
        # value depends on how many initialize polls the launcher needed.
        if "id" in norm:
            norm["id"] = "<ID>"
        transcript.append({"step": step, "envelope": norm})
        return envelope

    # ---------------- Segment A: AI enabled --------------------------------
    project = common.copy_fixture_to_temp(work_root)
    common.kill_stale_instances()
    app = common.App(exe, common.fixture_wfs(project), ai_enabled=True)
    try:
        # wait_for_mcp performs the real initialize; re-issue one explicitly
        # so the transcript owns a deterministic envelope.
        app.wait_for_mcp()
        app.wait_for_oscquery()   # project fully loaded

        record("initialize", app.mcp("initialize", {
            "protocolVersion": common.MCP_PROTOCOL_VERSION,
            "capabilities": {},
            "clientInfo": {"name": "control-replay", "version": "1"},
        }))

        # ---- protocol version negotiation ----
        # The server accepts three revisions and echoes back whatever the
        # client asked for when it can honour it, falling back to its latest
        # otherwise. An unsupported value in the MCP-Protocol-Version header
        # is rejected at the transport with HTTP 400.
        negotiation = {}
        for requested in ("2025-06-18", "2025-03-26", "2024-11-05", "1999-01-01"):
            got = app.mcp("initialize", {
                "protocolVersion": requested,
                "capabilities": {},
                "clientInfo": {"name": "control-replay", "version": "1"},
            })["result"]["protocolVersion"]
            negotiation[requested] = got
        negotiation["<omitted>"] = app.mcp("initialize", {
            "capabilities": {},
            "clientInfo": {"name": "control-replay", "version": "1"},
        })["result"]["protocolVersion"]

        expected_negotiation = {
            "2025-06-18": "2025-06-18",
            "2025-03-26": "2025-03-26",
            "2024-11-05": "2024-11-05",   # older clients keep working
            "1999-01-01": common.MCP_PROTOCOL_VERSION,
            "<omitted>":  common.MCP_PROTOCOL_VERSION,
        }
        if negotiation != expected_negotiation:
            hard_failures.append(f"protocol negotiation wrong: {negotiation}")

        bad_header_status = app.mcp_status_with_header(
            "MCP-Protocol-Version", "2020-01-01")
        if bad_header_status != 400:
            hard_failures.append(
                f"unsupported MCP-Protocol-Version header returned "
                f"{bad_header_status}, want 400")

        transcript.append({"step": "protocol_negotiation",
                           "negotiated": negotiation,
                           "unsupported_header_status": bad_header_status})

        # ---- tools/list census ----
        # The ~393 auto-generated per-parameter tools are registered but not
        # advertised (see shouldListGeneratedTool in MCPGeneratedToolLoader):
        # listing them cost ~240KB per connection. What must stay visible is
        # the hand-written surface plus every tier-3 generated tool, because
        # wfs_set_parameter refuses tier-3 writes by design.
        tl = app.mcp("tools/list")
        tools = tl["result"]["tools"]
        tiers = [t["_meta"]["tier"] for t in tools]
        names = [t["name"] for t in tools]
        count = len(tools)
        if not (25 <= count <= 60):
            hard_failures.append(
                f"tools/list count {count} outside expected visible surface "
                f"25..60 - did the hide policy change?")
        ordering_ok = all(
            (tiers[i] > tiers[i + 1])
            or (tiers[i] == tiers[i + 1] and names[i] < names[i + 1])
            for i in range(count - 1))
        if not ordering_ok:
            hard_failures.append("tools/list tier ordering contract violated "
                                 "(want tier DESC, name ASC)")

        # Hidden tools must remain callable — that is what makes hiding them
        # safe. Asserted for real further down (hidden_tool_still_callable).
        hidden_expected = ("input_position_set_x", "input_position_nudge_x")
        for n in hidden_expected:
            if n in names:
                hard_failures.append(f"{n} should be hidden from tools/list")

        census = {
            "count_in_visible_range": 25 <= count <= 60,
            "tier_ordering_ok": ordering_ok,
            "tier_counts": {str(t): tiers.count(t) for t in sorted(set(tiers))},
            "_meta": common.normalize_envelope(tl["result"].get("_meta", {})),
            "spot_check_present": {
                n: (n in names)
                for n in ("session_save", "session_get_state",
                          "wfs_set_parameter", "wfs_set_parameter_batch",
                          "wfs_nudge_parameter",
                          "mcp_undo_last_ai_change",
                          "system_i_o_set_input_channels")
            },
            "spot_check_hidden": {n: (n not in names) for n in hidden_expected},
        }
        transcript.append({"step": "tools_list_census", "census": census})

        # ---- hidden generated tool is still callable by name ----
        hidden_call = record("hidden_tool_still_callable",
                             app.tool("input_position_nudge_x",
                                      {"input_id": 3, "direction": "inc",
                                       "amount": 1.0}))
        if common.envelope_result(hidden_call).get("isError"):
            hard_failures.append(
                "hidden generated tool was not callable by name")

        # ---- tier-1 write + read-back ----
        record("tier1_write",
               app.tool("input_position_set_x", {"input_id": 1, "value": -5.0}))
        rb = app.tool("wfs_get_parameter",
                      {"variable": "inputPositionX", "channel_id": 1})
        record("tier1_readback", rb)
        rb_payload = common.tool_payload(rb)
        if not (isinstance(rb_payload, dict)
                and float(rb_payload.get("value", 0)) == -5.0):
            hard_failures.append(f"tier-1 read-back wrong: {rb_payload}")

        # ---- tier-2 confirm round-trip ----
        first, final = app.tool_confirmed("input_set_attenuation",
                                          {"input_id": 2, "db": -3.0})
        record("tier2_awaiting_confirmation", first)
        record("tier2_confirmed", final)
        tier_info = common.envelope_result(first).get("tier_enforcement", {})
        if not tier_info.get("awaiting_confirmation"):
            hard_failures.append("tier-2 first call did not return "
                                 "awaiting_confirmation")
        final_payload = common.tool_payload(final)
        if not (isinstance(final_payload, dict)
                and final_payload.get("db") == -3.0):
            hard_failures.append(f"tier-2 confirmed exec wrong: {final_payload}")

        # ---- tier-3: safety gate is UI-only, must refuse ----
        t3 = record("tier3_gate_closed",
                    app.tool("system_i_o_set_input_channels", {"value": 8}))
        t3_info = common.envelope_result(t3).get("tier_enforcement", {})
        if not t3_info.get("safety_gate_closed"):
            hard_failures.append(f"tier-3 call was not gate-refused: {t3_info}")

        # ---- 16-write batch as ONE undo step ----
        before = read_positions(app)
        transcript.append({"step": "batch_before", "values": before})

        b_first, b_final = app.tool_confirmed("wfs_set_parameter_batch",
                                              {"writes": BATCH_WRITES})
        record("batch_awaiting_confirmation", b_first)
        record("batch_confirmed", b_final)

        after = read_positions(app)
        transcript.append({"step": "batch_after", "values": after})
        expected_after = [("inputPositionX", i, i * 0.25) for i in range(1, 9)] \
            + [("inputPositionY", i, i * 0.5) for i in range(1, 9)]
        if after != expected_after:
            hard_failures.append(f"batch did not apply all 16 writes: {after}")

        record("undo", app.tool("mcp_undo_last_ai_change", {}))
        after_undo = read_positions(app)
        transcript.append({"step": "after_undo", "values": after_undo})
        if after_undo != before:
            hard_failures.append(
                "undo did NOT revert the 16-write batch as one step "
                f"(before={before}, after_undo={after_undo})")

        record("redo", app.tool("mcp_redo_last_undone_ai_change", {}))
        after_redo = read_positions(app)
        transcript.append({"step": "after_redo", "values": after_redo})
        if after_redo != after:
            hard_failures.append("redo did not re-apply the batch")

        record("history_compact",
               app.tool("mcp_get_ai_change_history", {"compact": True}))

        # ---- array attenuation: one family tool, ten parameters ----------
        # The generator collapses inputArrayAtten1..10 into ONE tool whose
        # `array` argument picks the member, and writes a template
        # ("inputArrayAtten{array}") where the variable would be. The loader
        # used to skip that tool and the registry to know only the literal
        # template, so MCP reached none of the ten. Placed after the batch
        # undo/redo and before the generic nudge: the session_save check
        # further down expects the nudge to be the last undoable change.
        def get_level(array: int, channel: int):
            payload = common.tool_payload(app.tool(
                "wfs_get_parameter",
                {"variable": f"inputArrayAtten{array}", "channel_id": channel}))
            return (round(float(payload["value"]), 6)
                    if isinstance(payload, dict) and "value" in payload
                    else payload)

        aa_first, aa_final = app.tool_confirmed(
            "input_set_array_attenuation",
            {"input_id": 3, "array": 4, "value": -6.5})
        record("array_atten_awaiting_confirmation", aa_first)
        record("array_atten_confirmed", aa_final)
        aa_payload = common.tool_payload(aa_final)
        if not (isinstance(aa_payload, dict)
                and aa_payload.get("variable") == "inputArrayAtten4"
                and aa_payload.get("array") == 4
                and float(aa_payload.get("value", 0)) == -6.5):
            hard_failures.append(
                f"input_set_array_attenuation did not write array 4: {aa_payload}")

        levels = {"input3_array4": get_level(4, 3),
                  "input3_array3": get_level(3, 3),
                  "input3_array5": get_level(5, 3),
                  "input2_array4": get_level(4, 2)}
        transcript.append({"step": "array_atten_readback", "values": levels})
        if levels != {"input3_array4": -6.5, "input3_array3": 0.0,
                      "input3_array5": 0.0, "input2_array4": 0.0}:
            hard_failures.append(
                f"array attenuation landed on the wrong member: {levels}")

        _, aa_generic = app.tool_confirmed(
            "wfs_set_parameter",
            {"variable": "inputArrayAtten10", "channel_id": 3, "value": -60.0})
        record("array_atten_generic_set", aa_generic)
        if get_level(10, 3) != -60.0:
            hard_failures.append("wfs_set_parameter did not write inputArrayAtten10")

        for label, call_args in (
                ("array_11", {"input_id": 3, "array": 11, "value": -3.0}),
                ("array_0", {"input_id": 3, "array": 0, "value": -3.0}),
                ("array_2_5", {"input_id": 3, "array": 2.5, "value": -3.0}),
                ("array_missing", {"input_id": 3, "value": -3.0}),
                ("value_minus_75", {"input_id": 3, "array": 2, "value": -75.0})):
            _, refused = app.tool_confirmed("input_set_array_attenuation",
                                            call_args)
            record(f"array_atten_refused_{label}", refused)
            if not common.envelope_result(refused).get("isError"):
                hard_failures.append(
                    f"input_set_array_attenuation accepted {call_args}")
        _, aa_generic_oor = app.tool_confirmed(
            "wfs_set_parameter",
            {"variable": "inputArrayAtten2", "channel_id": 3, "value": 3.0})
        record("array_atten_generic_refused", aa_generic_oor)
        if not common.envelope_result(aa_generic_oor).get("isError"):
            hard_failures.append("wfs_set_parameter accepted inputArrayAtten2 = +3 dB")
        if get_level(2, 3) != 0.0:
            hard_failures.append("a refused array attenuation write changed the level")

        described = record("array_atten_describe",
                           app.tool("mcp_describe_parameters",
                                    {"prefix": "inputArrayAtten", "mode": "full"}))
        described_payload = common.tool_payload(described)
        members = (described_payload.get("parameters") or []) \
            if isinstance(described_payload, dict) else []
        by_name = {m.get("variable"): m for m in members}
        want_names = {f"inputArrayAtten{n}" for n in range(1, 11)}
        if set(by_name) != want_names or any(
                by_name[f"inputArrayAtten{n}"].get("tool_args") != {"array": n}
                or by_name[f"inputArrayAtten{n}"].get("tool_name")
                != "input_set_array_attenuation"
                or f"Array {n}" not in by_name[f"inputArrayAtten{n}"].get("description", "")
                for n in range(1, 11)):
            hard_failures.append(
                f"describe did not list the ten array attenuations with their "
                f"tool_args: {sorted(by_name)}")

        _, aa_batch = app.tool_confirmed("wfs_set_parameter_batch", {"writes": [
            {"variable": "inputArrayAtten1", "channel_id": 5, "value": -3.0},
            {"variable": "inputArrayAtten2", "channel_id": 5, "value": -9.0}]})
        record("array_atten_batch", aa_batch)
        if (get_level(1, 5), get_level(2, 5)) != (-3.0, -9.0):
            hard_failures.append("wfs_set_parameter_batch did not write two array levels")

        # Undo resolves the input number through the registry record of the
        # variable it restores, which a member of the family now has
        record("array_atten_undo_batch", app.tool("mcp_undo_last_ai_change", {}))
        if (get_level(1, 5), get_level(2, 5)) != (0.0, 0.0):
            hard_failures.append("undo did not revert the array-level batch")
        record("array_atten_undo_set", app.tool("mcp_undo_last_ai_change", {}))
        if get_level(10, 3) != 0.0:
            hard_failures.append("undo did not revert the inputArrayAtten10 write")

        # ---- generic tools now carry the validation the named tools had ----
        # wfs_set_parameter used to range-check only against the permissive
        # OSCParameterBounds table; it now also honours the registry's
        # declared min/max and enum membership. Both must be refused.
        _, oor = app.tool_confirmed(
            "wfs_set_parameter",
            {"variable": "inputPositionX", "value": 99999.0, "channel_id": 1})
        record("generic_set_out_of_range", oor)
        oor_res = common.envelope_result(oor)
        if not oor_res.get("isError"):
            hard_failures.append(
                "wfs_set_parameter accepted an out-of-registry-range value")

        _, bad_enum = app.tool_confirmed(
            "wfs_set_parameter", {"variable": "stageShape", "value": 47})
        record("generic_set_bad_enum", bad_enum)
        if not common.envelope_result(bad_enum).get("isError"):
            hard_failures.append(
                "wfs_set_parameter accepted an out-of-enum value")

        # ---- generic nudge (replaces the now-hidden per-parameter nudges) --
        nudged = record("generic_nudge",
                        app.tool("wfs_nudge_parameter",
                                 {"variable": "inputPositionY",
                                  "direction": "dec", "amount": 0.5,
                                  "channel_id": 1}))
        if common.envelope_result(nudged).get("isError"):
            hard_failures.append("wfs_nudge_parameter failed on a tier-1 param")

        # ---- describe_parameters: group overview + summary/full modes ------
        groups = record("describe_groups",
                        app.tool("mcp_describe_parameters", {}))
        groups_payload = common.tool_payload(groups)
        if not (isinstance(groups_payload, dict)
                and groups_payload.get("view") == "groups"):
            hard_failures.append(
                f"unfiltered mcp_describe_parameters did not return the group "
                f"overview: {str(groups_payload)[:120]}")
        record("describe_summary",
               app.tool("mcp_describe_parameters",
                        {"group_key": "input_position"}))

        # ---- session_save leaves a non-undoable audit record ---------------
        # The save writes into the per-run temp copy of the fixture, not the
        # repo, because App() runs against copy_fixture_to_temp().
        _, saved = app.tool_confirmed("session_save", {})
        record("session_save", saved)
        if common.envelope_result(saved).get("isError"):
            hard_failures.append("session_save failed")

        hist = app.tool("mcp_get_ai_change_history", {"limit": 1,
                                                      "compact": False})
        record("history_after_save", hist)
        hist_payload = common.tool_payload(hist)
        newest = (hist_payload.get("records") or [{}])[-1] \
            if isinstance(hist_payload, dict) else {}
        if newest.get("tool_name") != "session_save":
            hard_failures.append(
                f"session_save left no history record (newest={newest.get('tool_name')})")
        if newest.get("undoable") is not False:
            hard_failures.append(
                "session_save record was not flagged non-undoable")

        # Undo must step OVER the save record and revert the nudge instead.
        y_before_undo = common.tool_payload(
            app.tool("wfs_get_parameter",
                     {"variable": "inputPositionY", "channel_id": 1}))
        undo_after_save = record("undo_skips_save_record",
                                 app.tool("mcp_undo_last_ai_change", {}))
        if common.envelope_result(undo_after_save).get("isError"):
            hard_failures.append("undo after session_save failed")
        y_after_undo = common.tool_payload(
            app.tool("wfs_get_parameter",
                     {"variable": "inputPositionY", "channel_id": 1}))
        if y_after_undo.get("value") == y_before_undo.get("value"):
            hard_failures.append(
                "undo after session_save changed nothing - it should have "
                "skipped the non-undoable save record and reverted the nudge")
    finally:
        app.close()

    # ---------------- Segment B: AI disabled -------------------------------
    project_b = common.copy_fixture_to_temp(work_root)
    app_b = common.App(exe, common.fixture_wfs(project_b), ai_enabled=False)
    try:
        app_b.wait_for_mcp()
        dis = record("ai_disabled",
                     app_b.tool("input_position_set_x",
                                {"input_id": 1, "value": -5.0}))
        dis_info = common.envelope_result(dis).get("tier_enforcement", {})
        if not dis_info.get("ai_disabled"):
            hard_failures.append(
                f"run without WFS_MCP_AI_ENABLED was not refused: {dis_info}")
    finally:
        app_b.close()

    if not args.keep_temp:
        shutil.rmtree(work_root, ignore_errors=True)

    ok = True
    for f in hard_failures:
        print(f"[mcp-replay] HARD FAIL: {f}", file=sys.stderr)
        ok = False

    actual_text = json.dumps(transcript, indent=2, sort_keys=True) + "\n"
    if not common.compare_or_update(GOLDEN, actual_text, args.update,
                                    "mcp-replay"):
        ok = False
    return common.EXIT_PASS if ok else common.EXIT_MISMATCH


if __name__ == "__main__":
    raise SystemExit(main())
