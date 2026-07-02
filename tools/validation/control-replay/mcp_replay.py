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
            "protocolVersion": "2024-11-05",
            "capabilities": {},
            "clientInfo": {"name": "control-replay", "version": "1"},
        }))

        # ---- tools/list census ----
        tl = app.mcp("tools/list")
        tools = tl["result"]["tools"]
        tiers = [t["_meta"]["tier"] for t in tools]
        names = [t["name"] for t in tools]
        count = len(tools)
        if count < 350:
            hard_failures.append(f"tools/list count {count} < 350")
        ordering_ok = all(
            (tiers[i] > tiers[i + 1])
            or (tiers[i] == tiers[i + 1] and names[i] < names[i + 1])
            for i in range(count - 1))
        if not ordering_ok:
            hard_failures.append("tools/list tier ordering contract violated "
                                 "(want tier DESC, name ASC)")
        census = {
            "count_at_least_350": count >= 350,
            "tier_ordering_ok": ordering_ok,
            "tier_counts": {str(t): tiers.count(t) for t in sorted(set(tiers))},
            "_meta": common.normalize_envelope(tl["result"].get("_meta", {})),
            "spot_check_present": {
                n: (n in names)
                for n in ("session_save", "session_get_state",
                          "wfs_set_parameter", "wfs_set_parameter_batch",
                          "mcp_undo_last_ai_change",
                          "system_i_o_set_input_channels")
            },
        }
        transcript.append({"step": "tools_list_census", "census": census})

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
