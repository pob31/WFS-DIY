"""MCP coverage check: the effects channels, the Map's drawn shapes, input
order and numbers, and the stereo/effects fields of session_get_state.

Launches WFS-DIY (AI enabled) on a temp copy of the golden fixture rewritten
to carry three effect channels, then drives the tools added for those gaps
and reads every write back. Hard asserts only, no golden: the transcript
golden is mcp_replay's job.

  session_get_state   inputs carry type mono/stereo, channel_counts carries
                      stereo_inputs and effects, an effects list exists
  generated effect_*  a flat value (attenuation), a module instance (dyn 2
                      threshold leaves dyn 1 alone), an EQ band (eq 2 band 3
                      leaves eq 1 band 3 AND the output EQ alone), a delay tap,
                      an enum written by label stores its id (reverb model
                      ModulatedHall = 4, phaser stages "12" = 12, mod shape
                      sine = 1), a reverb TYPE expands its preset (RT60
                      changes) and undo puts the type AND the preset back,
                      a dead effect id is refused
  wfs_set_parameter   reaches a flat effect value, refuses a per-module one
                      and names the tool that can write it
  effect_get / chain  chain order set and refused when not a permutation,
                      module on, fx->fx send on/level and the diagonal refused,
                      undo of the chain order, effect_clear answers
  effect lifecycle    effect_create / effect_delete move the count
  input order         input_move and input_reorder rearrange the display order
                      keeping numbers; input_renumber stops at the safety gate
  map shapes          add (rectangle, polyline), list, edit, delete, clear,
                      bad colour / too few points refused

Usage:
  python mcp_coverage_check.py [--exe path] [--keep-temp]

Exit codes: 0 pass, 1 fail, 2 usage, 3 app failed to start.
"""

from __future__ import annotations

import argparse
import os
import shutil
import sys
from pathlib import Path

import common

EFFECT_CHANNELS = 3


class Checker:
    def __init__(self, app: common.App):
        self.app = app
        self.failures: list[str] = []
        self.passed = 0

    def call(self, name: str, args: dict | None = None, confirm: bool = False):
        """(payload, is_error) for one tool call; tier-2 confirmed when asked."""
        if confirm:
            _, env = self.app.tool_confirmed(name, args or {})
        else:
            env = self.app.tool(name, args or {})
        result = common.envelope_result(env)
        return common.tool_payload(env), bool(result.get("isError"))

    def ok(self, name: str, args: dict | None = None, confirm: bool = False):
        payload, err = self.call(name, args, confirm)
        if err:
            self.fail(f"{name}({args}) errored: {payload}")
        return payload

    def refused(self, label: str, name: str, args: dict, needle: str = "",
                confirm: bool = False) -> str:
        payload, err = self.call(name, args, confirm)
        text = payload if isinstance(payload, str) else str(payload)
        if not err:
            self.fail(f"{label}: {name}({args}) was accepted: {payload}")
        elif needle and needle not in text:
            self.fail(f"{label}: refusal does not mention {needle!r}: {text}")
        else:
            self.passed += 1
        return text

    def expect(self, label: str, cond: bool, detail: object = "") -> None:
        if cond:
            self.passed += 1
        else:
            self.fail(f"{label} {detail}")

    def fail(self, msg: str) -> None:
        self.failures.append(msg)
        print(f"  FAIL {msg}")

    def get(self, variable: str, channel_id: int | None = None):
        args = {"variable": variable}
        if channel_id is not None:
            args["channel_id"] = channel_id
        payload = self.ok("wfs_get_parameter", args)
        return payload.get("value") if isinstance(payload, dict) else None

    def undo(self) -> None:
        self.ok("mcp_undo_last_ai_change")


def module(effect: dict, token: str) -> dict:
    for m in effect.get("chain", []):
        if m.get("module") == token:
            return m
    return {}


def check_session(c: Checker) -> None:
    state = c.ok("session_get_state")
    counts = state.get("channel_counts", {})
    c.expect("channel_counts.effects", counts.get("effects") == EFFECT_CHANNELS, counts)
    c.expect("channel_counts.stereo_inputs present", "stereo_inputs" in counts, counts)
    c.expect("effects listed", len(state.get("effects", [])) == EFFECT_CHANNELS, state.get("effects"))
    types = {i.get("type") for i in state.get("inputs", [])}
    c.expect("every input has a type", types <= {"mono", "stereo"} and types, types)

    # The effects knowledge resource (spatcore catalog row + the app's .md).
    res = c.app.mcp("resources/read", {"uri": "wfs://knowledge/effects"})
    contents = (res.get("result") or {}).get("contents") or [{}]
    c.expect("knowledge_effects served", "Effects channels" in contents[0].get("text", ""), str(res)[:200])


def check_generated_effect_tools(c: Checker) -> None:
    c.ok("effect_channel_set_attenuation", {"effect_id": 1, "value": -6.0}, confirm=True)
    c.expect("effect attenuation landed", c.get("effectAttenuation", 1) == -6.0)

    # Module instance: dyn 2 only.
    before = module(c.ok("effect_get", {"effect_id": 1}), "dyn1")["parameters"]["effectDynCompThreshold"]
    c.ok("effect_fx_dyn_set_dyn_comp_threshold", {"effect_id": 1, "instance": 2, "value": -33.0})
    fx = c.ok("effect_get", {"effect_id": 1})
    c.expect("dyn2 threshold", module(fx, "dyn2")["parameters"]["effectDynCompThreshold"] == -33.0,
             module(fx, "dyn2"))
    c.expect("dyn1 untouched", module(fx, "dyn1")["parameters"]["effectDynCompThreshold"] == before)

    # EQ band: eq 2 band 3 only - not eq 1, not the output EQ.
    eq1_before = module(fx, "eq1")["bands"][2]["effectEQgain"]
    c.ok("effect_fx_eq_set_eq_gain", {"effect_id": 1, "instance": 2, "effect_band": 3, "value": 7.5})
    fx = c.ok("effect_get", {"effect_id": 1})
    c.expect("eq2 band3 gain", module(fx, "eq2")["bands"][2]["effectEQgain"] == 7.5, module(fx, "eq2")["bands"][2])
    c.expect("eq1 band3 untouched", module(fx, "eq1")["bands"][2]["effectEQgain"] == eq1_before)

    # Delay tap 4.
    c.ok("effect_fx_delay_set_delay_tap_time", {"effect_id": 1, "tap": 4, "value": 640.0})
    fx = c.ok("effect_get", {"effect_id": 1})
    c.expect("tap 4 time", module(fx, "delay")["taps"][3]["effectDelayTapTime"] == 640.0, module(fx, "delay")["taps"][3])

    # Enum labels store their ids, not their positions.
    c.ok("effect_fx_reverb_set_reverb_model", {"effect_id": 2, "mode": "ModulatedHall"})
    c.expect("reverb model id 4", c.get("effectReverbModel", 2) == 4, c.get("effectReverbModel", 2))
    c.ok("effect_fx_phaser_set_phaser_stages", {"effect_id": 2, "value": "12"})
    c.expect("phaser stages 12", c.get("effectPhaserStages", 2) == 12, c.get("effectPhaserStages", 2))
    c.ok("effect_fx_mod_set_mod_shape", {"effect_id": 2, "shape": "sine"})
    c.expect("mod shape sine = 1", c.get("effectModShape", 2) == 1, c.get("effectModShape", 2))
    c.refused("phaser stages 5 not a stage count", "effect_fx_phaser_set_phaser_stages",
              {"effect_id": 2, "value": 5}, "invalid_enum_value")

    # Reverb TYPE expands its preset; undo restores type and preset.
    rt_before = c.get("effectReverbRT60", 3)
    type_before = c.get("effectReverbType", 3)
    c.ok("effect_fx_reverb_set_reverb_type", {"effect_id": 3, "value": "StoneCathedral"})
    rt_after = c.get("effectReverbRT60", 3)
    c.expect("reverb type 13", c.get("effectReverbType", 3) == 13, c.get("effectReverbType", 3))
    c.expect("preset changed RT60", rt_after != rt_before, (rt_before, rt_after))
    c.undo()
    c.expect("undo type", c.get("effectReverbType", 3) == type_before, c.get("effectReverbType", 3))
    c.expect("undo preset RT60", c.get("effectReverbRT60", 3) == rt_before, (rt_before, c.get("effectReverbRT60", 3)))

    c.refused("dead effect id", "effect_channel_set_mute", {"effect_id": EFFECT_CHANNELS + 1, "value": 1},
              "not a live channel")

    # wfs_set_parameter: flat yes, per-module no.
    c.ok("wfs_set_parameter", {"variable": "effectPositionX", "channel_id": 2, "value": 3.25}, confirm=True)
    c.expect("set_parameter effect position", c.get("effectPositionX", 2) == 3.25)
    c.refused("set_parameter per-module", "wfs_set_parameter",
              {"variable": "effectEQgain", "channel_id": 1, "value": 3.0}, "effect_fx_eq_set_eq_gain",
              confirm=True)


def check_hand_written_effect_tools(c: Checker) -> None:
    order = ["delay", "reverb", "eq1", "eq2", "dyn1", "dyn2", "dist", "mod", "phaser", "trem", "crush"]
    c.ok("effect_set_chain_order", {"effect_id": 1, "order": order})
    fx = c.ok("effect_get", {"effect_id": 1})
    c.expect("chain order", fx.get("chain_order") == ",".join(order), fx.get("chain_order"))
    c.expect("chain listed in playing order", [m["module"] for m in fx["chain"]] == order)
    c.undo()
    c.expect("chain order undone", c.ok("effect_get", {"effect_id": 1}).get("chain_order") != ",".join(order))
    c.refused("not a permutation", "effect_set_chain_order", {"effect_id": 1, "order": order[:-1] + ["delay"]},
              "exactly once")

    c.ok("effect_set_module_on", {"effect_id": 1, "module": "reverb", "on": True})
    c.expect("reverb on", module(c.ok("effect_get", {"effect_id": 1}), "reverb").get("on") is True)

    c.ok("effect_set_fx_send_on", {"effect_id": 2, "source_effect_id": 1, "on": True})
    c.ok("effect_set_fx_send_level", {"effect_id": 2, "source_effect_id": 1, "level_db": -12.0}, confirm=True)
    fx2 = c.ok("effect_get", {"effect_id": 2})
    c.expect("fed by effect 1", fx2.get("fed_by_effects") == [{"effect_id": 1, "level_db": -12.0}],
             fx2.get("fed_by_effects"))
    c.expect("effect 1 feeds 2", c.ok("effect_get", {"effect_id": 1}).get("feeds_effects")
             == [{"effect_id": 2, "level_db": -12.0}])
    c.refused("diagonal", "effect_set_fx_send_on", {"effect_id": 2, "source_effect_id": 2, "on": True}, "itself")

    payload = c.ok("effect_clear", {"effect_id": 1})
    c.expect("effect_clear answered", isinstance(payload, dict) and payload.get("cleared") == 1, payload)


def check_effect_lifecycle(c: Checker) -> None:
    created = c.ok("effect_create", {}, confirm=True)
    c.expect("effect created", isinstance(created, dict) and created.get("total") == EFFECT_CHANNELS + 1, created)
    deleted = c.ok("effect_delete", {}, confirm=True)
    c.expect("effect deleted", isinstance(deleted, dict) and deleted.get("total") == EFFECT_CHANNELS, deleted)


def check_input_order(c: Checker) -> None:
    ids = [i["id"] for i in c.ok("session_get_state")["inputs"]]
    if len(ids) < 3:
        c.fail(f"fixture needs at least 3 inputs, has {ids}")
        return

    moved = c.ok("input_move", {"input_id": ids[0], "position": 3}, confirm=True)
    new_order = [i["id"] for i in moved.get("order", [])]
    c.expect("input_move", new_order[:3] == [ids[1], ids[2], ids[0]], new_order)
    c.expect("numbers kept", sorted(new_order) == sorted(ids))

    reordered = c.ok("input_reorder", {"order": ids}, confirm=True)
    c.expect("input_reorder restores", [i["id"] for i in reordered.get("order", [])] == ids, reordered)
    c.refused("duplicate in order", "input_reorder", {"order": [ids[0], ids[0]]}, "twice", confirm=True)

    # Tier 3: the safety gate is operator-only, so the call must stop there.
    payload, err = c.call("input_renumber", {"numbers": list(reversed(ids))})
    text = str(payload)
    c.expect("input_renumber gated", err and "Critical AI actions are blocked" in text, text[:200])
    c.expect("numbers unchanged", [i["id"] for i in c.ok("session_get_state")["inputs"]] == ids)


def check_map_shapes(c: Checker) -> None:
    start = c.ok("map_shapes_list").get("count", 0)
    rect = c.ok("map_shape_add", {"type": "rectangle", "x": 1, "y": 2, "width": 4, "height": 2,
                                  "colour": "amber", "label": "Stage"})
    c.expect("rectangle index", rect.get("index") == start + 1, rect)
    c.expect("rectangle bounds", rect.get("bounds") == {"min_x": -1.0, "min_y": 1.0, "max_x": 3.0, "max_y": 3.0},
             rect.get("bounds"))
    line = c.ok("map_shape_add", {"type": "polyline", "points": [[0, 0], [5, 5]], "dashed": True})
    c.expect("polyline points", line.get("points") == [[0.0, 0.0], [5.0, 5.0]], line.get("points"))

    edited = c.ok("map_shape_edit", {"index": rect["index"], "x": 0, "colour": "#112233", "label": ""})
    c.expect("edit moved", edited.get("x") == 0.0 and edited.get("colour") == "#112233"
             and "label" not in edited, edited)

    listed = c.ok("map_shapes_list")
    c.expect("list count", listed.get("count") == start + 2, listed.get("count"))

    c.refused("bad colour", "map_shape_add", {"type": "ellipse", "x": 0, "y": 0, "width": 1, "height": 1,
                                              "colour": "puce"}, "colour")
    c.refused("too few points", "map_shape_add", {"type": "polygon", "points": [[0, 0], [1, 1]]}, "at least 3")

    c.ok("map_shape_delete", {"index": line["index"]})
    c.expect("deleted", c.ok("map_shapes_list").get("count") == start + 1)
    cleared = c.ok("map_shapes_clear", {}, confirm=True)
    c.expect("cleared", c.ok("map_shapes_list").get("count") == 0, cleared)


TRACKING_PORT = 7000      # trackingPort in the fixture
TARGET_LISTEN_PORT = 9123  # a UDP port this driver listens on as an OSC target


def check_group1(c: Checker) -> None:
    """Audit 2026-11-24 group 1: tools that answered ok and changed nothing."""
    import socket
    import time

    # -- network target lifecycle + writes reach the live OSC engine --------
    listed = c.ok("network_targets_list")
    before = len(listed.get("targets", []))
    created = c.ok("network_target_create", {}, confirm=True)
    tid = created.get("target_id")
    c.expect("target created", isinstance(tid, int) and len(c.ok("network_targets_list")["targets"]) == before + 1, created)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("127.0.0.1", TARGET_LISTEN_PORT))
    sock.settimeout(0.5)
    c.ok("network_set_t_sip", {"target_id": tid, "value": "127.0.0.1"})
    c.ok("network_set_t_sport", {"target_id": tid, "value": TARGET_LISTEN_PORT})
    c.ok("network_set_ts_protocol", {"target_id": tid, "protocol": "OSC"})
    c.ok("network_set_t_stx_enable", {"target_id": tid, "value": "ON"})
    time.sleep(0.5)
    got = 0
    deadline = time.monotonic() + 4.0
    x = 1.0
    while time.monotonic() < deadline and got == 0:
        x += 0.25
        c.ok("input_position_set_x", {"input_id": 1, "value": x})
        try:
            data, _ = sock.recvfrom(65536)
            got = len(data)
        except socket.timeout:
            pass
    sock.close()
    c.expect("target configured over MCP receives OSC (live engine applied)", got > 0)
    c.expect("target_id addresses the row", any(t["target_id"] == tid and t["port"] == TARGET_LISTEN_PORT
                                               for t in c.ok("network_targets_list")["targets"]))
    deleted = c.ok("network_target_delete", {"target_id": tid}, confirm=True)
    c.expect("target deleted", len(deleted.get("targets", [None] * 99)) == before, deleted)

    # -- tracking enabled over MCP starts the receiver -----------------------
    c.ok("input_tracking_set_active", {"input_id": 1, "value": 1})
    c.ok("network_tracking_set_protocol", {"protocol": "OSC"}, confirm=True)
    c.ok("network_tracking_set_enabled", {"value": "ON"}, confirm=True)
    time.sleep(1.0)
    tracker = common.OSCSender(port=TRACKING_PORT, delay=0.3)
    tracker.send("/wfs/tracking", [("i", 1), ("f", 1.25), ("f", 0.5), ("f", 0.0)])
    tracker.close()
    time.sleep(1.0)
    c.expect("tracking receiver started by MCP", c.get("inputOffsetX", 1) == 1.25, c.get("inputOffsetX", 1))

    # -- Apply to Array -------------------------------------------------------
    for o in (1, 2, 3):
        c.ok("output_set_array", {"output_id": o, "value": "Array1"})
        c.ok("output_array_set_apply_to_array", {"output_id": o, "value": "ABSOLUTE"})
    c.ok("output_set_attenuation", {"output_id": 1, "value": -7.0}, confirm=True)
    c.expect("array member follows", c.get("outputAttenuation", 3) == -7.0, c.get("outputAttenuation", 3))
    c.undo()
    c.expect("undo reverts the member too", c.get("outputAttenuation", 3) != -7.0, c.get("outputAttenuation", 3))
    c.ok("output_set_attenuation", {"output_id": 1, "value": -9.0, "this_output_only": True}, confirm=True)
    c.expect("this_output_only leaves the array", c.get("outputAttenuation", 2) != -9.0, c.get("outputAttenuation", 2))

    # -- per-output mutes and macros -----------------------------------------
    m = c.ok("reverb_set_output_mute", {"reverb_id": 1, "output_id": 2, "muted": True})
    c.expect("reverb output mute", m.get("muted_outputs") == [2], m)
    m = c.ok("reverb_apply_mute_macro", {"reverb_id": 1, "macro": "invert"})
    c.expect("reverb invert", 2 not in m.get("muted_outputs", []) and 1 in m.get("muted_outputs", []), m)
    c.undo()
    m = c.ok("effect_apply_mute_macro", {"effect_id": 1, "macro": "mute_array", "array": 1})
    c.expect("effect mute_array", m.get("muted_outputs") == [1, 2, 3], m)
    m = c.ok("input_apply_mute_macro", {"input_id": 1, "macro": "mute_odd"})
    c.expect("input mute_odd", m.get("muted_outputs", [])[:3] == [1, 3, 5], m)
    c.undo()

    # -- gradient layer white/black in the layer's unit ----------------------
    c.ok("input_gradient_map_set_gm_layer_white", {"input_id": 1, "layer": 0, "value": -40.0})
    c.refused("attenuation layer refuses +10", "input_gradient_map_set_gm_layer_white",
              {"input_id": 1, "layer": 0, "value": 10.0}, "layer 0")
    c.ok("input_gradient_map_set_gm_layer_black", {"input_id": 1, "layer": 1, "value": 12.0})

    # -- sampler set cells, controller mode -----------------------------------
    s = c.ok("input_sampler_create_set", {"input_id": 1}, confirm=True)
    sid = s.get("set_id")
    cells = c.ok("input_sampler_set_set_cells", {"input_id": 1, "set_id": sid, "cells": [5, 0, 5, 12]})
    c.expect("sampler set cells", cells.get("cells") == [0, 5, 12], cells)
    c.ok("system_controllers_set_sampler_controller_mode", {"mode": "Remote"})
    time.sleep(0.5)
    c.expect("sampler controller mode", c.get("SamplerControllerMode") == 2, c.get("SamplerControllerMode"))

    # -- preset names, live IP -------------------------------------------------
    c.ok("cluster_lfo_preset_set_name", {"preset": 16, "name": "Swirl"})
    names = c.ok("cluster_lfo_presets_list").get("presets", [])
    c.expect("preset 16 named", len(names) == 16 and names[15].get("name") == "Swirl", names[-1:])
    g = c.ok("session_get_global_state", {"sections": ["network"]})
    c.expect("live IPv4 listed", isinstance(g.get("local_ipv4_addresses"), list), list(g)[:6])


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--exe", default=None)
    p.add_argument("--keep-temp", action="store_true")
    args = p.parse_args()

    exe = common.find_exe(args.exe)
    work_root = Path(os.environ.get("TEMP", ".")) / "wfs-control-replay" / "mcp_coverage"
    project = common.copy_fixture_to_temp(work_root)
    common.set_fixture_effect_channels(project, EFFECT_CHANNELS)
    common.kill_stale_instances()
    app = common.App(exe, common.fixture_wfs(project), ai_enabled=True)
    c = Checker(app)
    try:
        app.wait_for_mcp()
        app.wait_for_oscquery()
        for name, fn in [("session", check_session),
                         ("generated effect tools", check_generated_effect_tools),
                         ("hand-written effect tools", check_hand_written_effect_tools),
                         ("effect lifecycle", check_effect_lifecycle),
                         ("input order", check_input_order),
                         ("map shapes", check_map_shapes),
                         ("audit group 1", check_group1)]:
            print(f"[coverage] {name}")
            try:
                fn(c)
            except Exception as exc:   # one section's crash must not hide the others
                c.fail(f"{name} raised {type(exc).__name__}: {exc}")
    finally:
        app.close()
        if not args.keep_temp:
            shutil.rmtree(work_root, ignore_errors=True)

    print(f"[coverage] {c.passed} checks passed, {len(c.failures)} failed")
    return 0 if not c.failures else 1


if __name__ == "__main__":
    sys.exit(main())
