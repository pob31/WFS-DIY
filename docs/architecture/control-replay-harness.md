# Control-Plane Replay Harnesses — Design (Phase 0 of the spatcore extraction)

Status: **implemented** at `tools/validation/control-replay/` (Python, stdlib
only — repo convention per `tools/fuzz/`). Three drivers gate the control
plane through the LIVE app (state/OSC/MCP are not yet extractable):
session round-trip, OSC replay, MCP transcript replay
(`core-boundary-proposal-control.md` §7). All three pass repeatably from
clean fixture copies.

Implementation corrections to this design:
- The assumed MCP session-save tool did not exist — a minimal **tier-2
  `session_save` tool** was added (`tools/SessionTools.h`, calls the same
  `saveCompleteConfig()` as the SystemConfig Save button).
- `saveCompleteConfig` writes **5** section files, not 7 — `show.xml` and
  `audio_patch.xml` are legacy getters; the audio patch is embedded in
  `system.xml`. The round-trip diffs the 5 real files.
- The fixture's network config (OSCQuery on, one localhost OSC target) had to
  be hand-edited then canonicalized through a load→save cycle, because the
  generated `network_set_*` MCP tools are silent no-ops (defect list below).

App defects surfaced by the harness (fixed: #1; open: #2-#5):
1. **FIXED** — `WFSFileManager::mergeTreeRecursive` matched children by id
   only, so `ADMCartMapping`/`ADMPolarMapping` (shared ids 0-3) duplicated 4
   nodes on every `network.xml` load; load→save was never a fixed point.
2. Generated `network_set_t_s*` / `network_set_osc_query_enabled` MCP tools
   silently no-op: `WFSValueTreeState::getTreeForParameter` never resolves
   `NetworkTarget` children (writes vanish, success payload returned).
3. `input_set_attenuation` coerces a missing `db` argument to 0.0 dB instead
   of erroring — schema-required args are not validated server-side.
4. `OSCIngestQueue` coalescing keeps only the newest write per
   (address, channel) slot — rapid absolute-then-increment OSC sequences are
   lossy; the driver spaces sends as a workaround.
5. File-loaded numeric XML attributes stay string-typed in the ValueTree until
   first runtime write (visible in OSCQuery read-backs).

## Verified facts the drivers build on (file:line in code as of 2026-07-02)

- **Headless project load**: `WFS-DIY.exe "<folder>\<Project>.wfs"` — the whole
  command line is treated as a `.wfs` manifest path (`Main.cpp:252-260`),
  loaded async after the message loop starts (`Main.cpp:120-128`). The app
  boots fine with no audio device (`MainComponent.cpp:2261-2263`, `:4307`);
  OSC/OSCQuery/MCP servers start independent of audio (MCP at
  `MainComponent.cpp:843`, loopback-only, port 7400). Single-instance app —
  kill stale instances first. Prefer graceful close (WM_CLOSE) so
  `cleanShutdown=true` is written and no recovery prompt appears next launch.
- **Session files**: `show/system/inputs/outputs/reverbs/audio_patch/network
  .xml` + `<Folder>.wfs` manifest (`WFSFileManager.cpp:202-241,185-188`).
  Diff normalization: strip the `<!-- Created: ... -->` header comment line
  (`:2044-2053`) in every XML; normalize `.wfs` `appVersion`+`createdDate`
  (`:129-130`); ignore `backups\`.
- **OSC**: UDP 8000 / TCP 8001 (4-byte length-prefixed), TX only to configured
  targets (no auto-ACK). State read-back surface = **OSCQuery HTTP**
  `GET /<path>?VALUE` → `{"VALUE":[...]}` (must be enabled in the fixture's
  `network.xml`). Out-of-range writes are rejected keep-current
  (`OSCParameterBounds`; file side `WFSFileManager.cpp:2062-2073`).
  Reuse `tools/fuzz/osc_codec.py` (encoder), `osc_fuzz.py` `Sender`/`LogTail`/
  `QueryClient`, and `addresses.py` (family inventory) directly.
- **MCP**: POST `http://127.0.0.1:7400/mcp`, JSON-RPC 2.0, protocol
  `2024-11-05`, no sessions. Envelope shapes + tier flows per
  `MCPDispatcher.cpp:87-373`; tier-2 confirm token lifetime 30 s, tier-3 gate
  300 s and UI-only. Normalize: `confirmation_token`, `expires_in_seconds`,
  `serverInfo.version`, `instructions`, `_meta.ai_enabled` /
  `critical_actions_allowed`, timestamps, `notifications[]`.

## The two headless blockers and the resolution

1. **MCP AI master toggle** is an in-memory atomic defaulting to **false**
   (`MCPTierEnforcement.h:148`), flipped only by the Network-tab UI button
   (`NetworkTab.h:1750-1754`) — never persisted, no OSC/CLI/config path.
   (The docstring at `MCPTierEnforcement.h:104` claiming "defaults to true" is
   wrong — trust the member.)
2. **No full-project save** over OSC or CLI — only per-input snapshots
   (`/wfs/input/snapshot/store`, `OSCManager.cpp:1670-1690`). Full save goes
   through MCP session tools or the UI.

**Resolution:** one minimal automation hook — at startup, after the MCP server
starts, if the environment variable **`WFS_MCP_AI_ENABLED=1`** is set, call
`tierEnforcement.setAIEnabled(true)` and log one line. That single hook makes
the MCP driver able to exercise tier-1/2 flows AND trigger full saves via the
MCP session tool, which also unblocks the session round-trip driver. Scope
deliberately excludes the tier-3 safety gate (stays UI-only; the transcript
asserts the `safety_gate_closed` envelope instead). Risk: MCP is loopback-only
and tier enforcement still applies; the env var is an explicit local opt-in
equivalent to clicking the UI toggle.

## Driver shapes

```
tools/validation/control-replay/
├── common.py            # app launcher (spawn exe + .wfs arg, env, poll ports,
│                        #   graceful WM_CLOSE), XML/JSON normalizers, differs
├── session_roundtrip.py # copy fixture project -> temp, launch, MCP save,
│                        #   close, diff all section XMLs vs fixture (normalized)
├── osc_replay.py        # scripted writes across families (/wfs/input|output|
│                        #   reverb|config, /cluster, /remoteInput, out-of-range
│                        #   rejects), read-back via OSCQuery, diff vs golden
├── mcp_replay.py        # transcript: initialize, tools/list census, tier-1
│                        #   write, tier-2 confirm + expiry-recovery envelope,
│                        #   tier-3 closed envelope, ai-disabled envelope (run
│                        #   without the env var), 16-write batch, undo-as-one,
│                        #   redo, history; diff normalized envelopes vs golden
├── fixtures/golden-project/   # committed minimal project folder (+ OSCQuery
│                              #   enabled + one localhost OSC target in
│                              #   network.xml)
└── goldens/             # committed expected outputs (normalized)
```

All three exit 0/1 with the same contract as `kernel_hashes.py`. Runner
prerequisite: a built `WFS-DIY.exe` (Release or Debug) — path auto-probed like
the offline-render plugin dir, overridable `--exe`.

## Open implementation details

- Golden fixture bootstrap: generate once by launching the app clean, applying
  a small deterministic scene over OSC/MCP, saving via the MCP session tool,
  then committing the folder (normalized).
- Identify the exact MCP session-save tool name from
  `Source/Network/MCP/tools/SessionTools.h` / `generated_tools.json` at
  implementation time, including its tier (a tier-2 save needs the confirm
  round-trip in the driver — that is fine, it exercises the flow).
- `%APPDATA%\WFS-DIY\WFS-DIY.settings` carries `lastProjectFolder` +
  `cleanShutdown` — drivers must not depend on it (always pass the `.wfs`
  path) but should leave `cleanShutdown=true` behind.
