# WFS-DIY re-audit (2026-09-29)

> **For the agent picking this up:** every file:line below refers to branch **`effects/reverb-models` @ `4c94e78f`** (spatcore submodule @ `9384e35`), not to `main`. Check that branch out, with `git submodule update --init` so spatcore is at that pin, before working through it. The first audit this compares against is `Documentation/WFS-DIY-audit_20260928.md` on that same branch. Items marked ✔ were checked in the code or reproduced; the others come from review agents and should be confirmed before fixing. CI changes were left out on purpose.

Branch `effects/reverb-models` @ `4c94e78f`, spatcore @ `9384e35`, compared with the 2026-09-28 audit (`Documentation/WFS-DIY-audit_20260928.md`). CI was left out on purpose.

## What was run

| Check | Result |
|---|---|
| Linux Debug build (`-Wall -Wextra`) | Builds and links. The only warnings in first-party code are the same benign ones as before. |
| spatcore unit tests | **All pass** |
| OSC parser under ASan+UBSan: the old 28-byte crash packet, a bundle nested 3,000 deep, and 300,000 mutated packets | **No crash and no sanitizer error.** The deep bundle is refused in about 2 ms. |
| `WFS_TEST_VALUE_GATES` self-test (Linux, under Xvfb) | **47 pass, 1 fail.** `"~home"` is refused as a snapshot name on POSIX; see R6. |
| `WFS_TEST_AUTOSTART_PROCESSING` + `WFS_TEST_ENGINE_RECONFIG` against a live dummy JACK device (64 in / 64 out) | **ALL PASS**: algorithm switch while running, ten grow/shrink cycles while processing, and ten binaural re-prepares while the binaural branch is live. |
| Five parallel reviews | One each for the network, MCP, engine and persistence fixes, plus a fresh bug hunt in the areas the first audit covered least. |

✔ = I checked this in the code myself or reproduced it; everything else is from a review agent.

---

## 1. Fix verification

| Audit item | Verdict | Notes |
|---|---|---|
| N1 OSC bundle overflow | **Complete** ✔ | Each element is parsed within its own bytes, every reader is guarded, depth is capped at 16. Fuzzed clean. |
| N2 text values bypass gain bounds | **Complete for gains.** Gaps remain elsewhere. | Input, output, reverb and Remote paths go through `readValueArgument`. The store refuses NaN, Inf and non-numeric text at every parameter with a range. The output stage clamps −92..0 dB. For the gaps, see B1 and B2. |
| S1 snapshot names as paths | **Complete** ✔ | `getNamedXmlFile` is used at every place a name becomes a path. Its parent-directory check also stops absolute paths and `~` expansion. |
| N6 tracking NaN | **Complete** | Non-finite samples are dropped in the ingest queue and the OSC receiver; the filter refuses NaN. |
| M1 nested JSON | **Complete** | The 64-level pre-scan runs before every JSON parse of network input (MCP, OSCQuery WebSocket, OSC string argument, MQTT). The update checker is the exception (Low). |
| M2 MCP transport | **Partial** | Host allow-list, Origin allow-list, `application/json` required and no more `ACAO: *`: DNS rebinding and ordinary web pages are blocked. There is still **no token**, so a page served from another loopback origin (a dev server, Jupyter) or any local process can still call tools. Real clients are unaffected in code; Claude Desktop's `url` mode wasn't tested live. |
| M4 non-finite MCP arguments | **Complete** | `readFiniteNumber` is used in every hand-written numeric tool and in nudge. Generated tools are covered by range checks and the store gate. |
| A1–A4 reconfiguring a running engine | **Complete** ✔ | `ScopedAudioStructureChange` silences the callback, the workers are joined before anything is freed, and every reshape path goes through `handleChannelCountChange`. Confirmed by the JACK self-test run. |
| G1 shutdown order | **Complete** | `parameters` is now declared before the tabs. |
| S2 save integrity | **Write path complete; "never without its backup" incomplete** | Temp file, checked write, flush (fsync), size check, rename. Three things are missing: **snapshot Store over an existing name (GUI and OSC) and template "Save as" still overwrite without a backup** (`SnapshotSession.h:605,618`, `MainComponent.cpp:2650`, `SnapshotScopeWindow.h:1336`); both autosaves ignore failures, which are not logged in Release because `setError` is DBG-only (`WFSFileManager.cpp:3905`); and the sampler set export ignores its result. |

---

## 2. Regressions introduced by the fixes

| # | Sev | Where | Issue | Fix |
|---|---|---|---|---|
| R1 | **Medium** ✔ | `OSCMessageRouter.cpp:1229-1233` → `OSCManager.cpp:2744` | **A gradient-map layer enable sent as text now switches the layer OFF.** `gmLayer*Enabled` has no bounds entry, so `"1"` stays a string and the handler's `isDouble() && >= 0.5` test fails. Before the fix it was coerced to a number. This hits QLab cues that send unquoted arguments. | In `readValueArgument`, coerce numeric text for any parameter that isn't a name, or add `BIND_BOOL` entries. |
| R2 | **Medium** | `MainComponent.cpp:16257`, `BinauralProcessor.h:75-95` | **Turning binaural monitoring on mid-show silences the whole PA for a few blocks.** The structure scope now covers `binauralProcessor->prepareToPlay`, which allocates about 49 MB of delay lines at 64 sources / 96 kHz while every output is muted. | Prepare the binaural buffers aside and swap them in, or gate only the binaural branch. |
| R3 | **Medium** | `WFSFileManager.cpp:390-424` | **A complete save can now leave files from different generations.** A section whose backup fails is skipped, but the other sections are still written. | Take all six backups first, and stop before the first write if any fails. |
| R4 | **Medium** | `handleConfigReloaded` 13326-13339 + `MCPUndoEngine.cpp:664-671` | **Tree and engine drift now turns the next cue into a processing stop.** An MCP undo of a channel create or delete (M3, still open) changes the count in the tree only. The next snapshot recall sees a shape change and stops processing mid-show. | Close M3; while processing, have recall warn rather than reshape. |
| R5 | Low-Med | `XmlPersistence.cpp:105,118` | **Backup names have one-second resolution, and a failed backup now blocks the save.** An autosave followed by Save in the same second, with Dropbox or antivirus holding the first backup, is refused. | Millisecond or unique backup names (`getNonexistentSibling`). |
| R6 | Low ✔ | `WFSFileManager.cpp:341-352`; self-test at `MainComponent.cpp:3509` | **On macOS and Linux a snapshot name starting with `~` is refused**, and the error message wrongly says the cause is `/`, `\` or `:`. It fails safe, but the self-test expects the name to work, so `WFS_TEST_VALUE_GATES` fails on POSIX. | Accept it (e.g. build the path with `folder.getFullPathName() + sep + name`), or document it and make the test platform-aware. |
| R7 | Low | `SnapshotSession.h:603-613` | **Store with QLab on carries on after a failed save**: it still exports the cue and clears the dirty marks. | Do what the update path does. |
| R8 | Low | `WFSFileManager.cpp:1341,2326` | **A leftover hidden temp file (`.<name>_tempXXXX.xml`) after a crash appears in the snapshot and template lists.** | Pass `ignoreHiddenFiles`. |
| R9 | Info | OSC input | `"inc"` sent to a parameter without an increment form is now refused. This is intended, but cue lists that relied on it will log refusals. | — |

## 3. Bypasses of the new value gates

| # | Sev | Where | Issue |
|---|---|---|---|
| B1 | Medium | `OSCManager.cpp:4242-4257` | **Cluster LFO amplitude, rate and phase from OSC** are written with a raw `setProperty`, so there is no range check and the store gate is skipped: `/wfs/cluster/lfoAmplitudeX 1 1e30` is stored as sent. |
| B2 | Low | `OSCParameterBounds.cpp` | These OSC-writable parameters still have no bounds entry, so any text or value is stored: `samplerSet`, `muteMacro`, `muteReverbSends`, `gmLayer0/1/2Enabled`, `applyToArray`, reverb `muteMacro`. Their readers convert to int, so no gain is exposed. |
| B3 | Medium (plausible) | `Plugin/Source/Master/MasterProcessor.cpp:261` | **The plugin decodes ADM-OSC with JUCE's own `OSCReceiver`, which has no nesting cap.** A 64 KB bundle nested about 3,000 deep may overflow the host's thread stack. |
| B4 | Low | MCP `reverbIRfile` and `samplerCellFile` (tier 1) | Absolute, `..` and UNC paths are still read (UNC leaks NTLM on Windows). |
| B5 | Low | `WFSFileManager::getNamedXmlFile` | Windows reserved device names (`CON`, `NUL`, `COM1`…) are not refused. |
| B6 | Low | `Source/UpdateChecker.h:74` | The update checker's reply is parsed with plain `JSON::parse`, without the depth pre-scan. |

---

## 4. New findings: fresh bug hunt

| # | Sev | Where | Issue | Fix |
|---|---|---|---|---|
| F1 | **High** ✔ | `CoordinateConverter.h:66-73`, `ArrayGeometryCalculator.cpp:10`, `Source/Shared/PluginAdmMapping.h:61-66` | **One OSC packet with a huge angle hangs the message thread forever.** `normalizeAngle` wraps with `while (d > 180) d -= 360`. Above about 8.6e9, subtracting 360 no longer changes a float. The value is finite, so the new gates pass it. It is reachable as `/wfs/input/positionTheta 1 1e10` (`OSCManager.cpp:2431`), through the ADM polar mapping, or by typing a huge azimuth. The whole UI and the control tick freeze. | `std::remainder(d, 360.f)` plus a `jlimit` on incoming angles. |
| F2 | **High** ✔ | `WFSCalculationEngine.cpp:1197` (outputs), `:1278` (reverb feeds), `:985` (effect returns) | **Setting Angle On to 90° or more switches off the speaker's rear-mute cone completely.** `if (angleOnDeg >= 90) return 1.0f` runs before the off zone (`angle >= π − angleOff`) is checked. At On 90 / Off 90 (one degree from the default 86/90), a source directly in front of the speaker plays at full level instead of muted. On 120 / Off 40 should mute a 40° front cone, but mutes nothing. | Early-out only when `angleOn >= 180 − angleOff`. |
| F3 | Med-High | `WFSValueTreeState.cpp:1365-1401` | **Moving an orientation dial past ±180° sends every relatively linked array member to ±180°.** The delta is `new − old` across the wrap (e.g. −350°), then the members are clamped instead of wrapped. | Wrap the delta and the result into ±180°. |
| F4 | Medium | `SamplerEngine.h:279-286`, `MainComponent.cpp:745-761` | **Previewing a sampler cell leaves the input moved by the cell's offset for good.** NoteOff never releases the position override. A tap shorter than one block re-enables it after release. | Release the override in the engine when playback ends. |
| F5 | Medium | `SamplerEngine.h:302-311` | **A quick tap on a cell with a long fade-in gives a full-level burst.** The fade-out starts from 1.0, not from the current envelope. | Start the fade-out from `envelopeGain`. |
| F6 | Medium | `LFOProcessor.h`, `ClusterLFOProcessor.h` | **Switching an LFO axis (or the cluster dip crossfade) to Off makes the source jump, up to the full amplitude in one tick.** The Off shape returns 0 at once, and the jump comes after the speed limiter. | Keep evaluating the previous shape while it fades out. |
| F7 | Medium | `AutomOtionProcessor.h:470-481` | **A relative polar AutomOtion of whole turns (θ +360, +720) is refused** as "destination is (0,0,0)", even though the θ range allows ±3600° for exactly that. | Skip the zero check in polar modes when Δθ or Δφ is not zero. |
| F8 | Medium | `InputsTab.h:6125-6172`, `OutputsTab.h:2195`, `ReverbTab.h:4797`, `SamplerSubTab.h:1099` | **Typed position fields use `getFloatValue()`, not `TypedValue`.** `"-2,5"` becomes −2 m, and a cleared field or a stray letter moves the source to the origin. | Read through `TypedValue::number` and put the old value back on failure. |
| F9 | Low-Med | `SamplerData.h:91-94` | **The sampler's "Height" pressure mapping and `pressXYScale` are shown and saved but never used.** | Apply them or hide them. |
| F10 | Low | `LiveSourceTamerEngine.h` | **Live Source Tamer radius 0 with a source exactly on a speaker gives 0/0 = NaN in the level matrix.** | Treat radius ≤ 0 as no reduction. |
| F11 | Low | `SamplerFileOps.h:89-97` | **Sample import overwrites the existing file after 999 name collisions.** | Fail instead. |
| F12 | Low (docs) | `Documentation/CLAUDE.md:180-195` | **The azimuth convention and formula in the docs don't match the code** (the code uses 0° = upstage, `atan2(x, y)`). | Correct the doc. |

---

## 5. Still open from the first audit (CI excluded)

**High**
- **N3:** a TCP OSC or MQTT peer that closes cleanly leaves a thread spinning at 100% CPU (`spatcore/control/osc/OSCTCPReceiver.cpp:291-296`, `TrackingMQTTReceiver.cpp:175/298`).
- **N4:** OSCQuery HTTP builds the tree on its asio threads (`OSCQueryServer.cpp:127` → `buildFullTree`).
- **N5:** tracking-OSC still queues `callAsync([this…])` for every datagram, with no bound (`OSCReceiverWithSenderIP.cpp:120`, `TrackingOSCReceiver.cpp:51-58`).
- **M3:** MCP undo of a channel create or delete bypasses the processing refusal and the channel-count funnel (`MCPUndoEngine.cpp:671`); see also R4.
- **P1:** the plugins still unregister from the bridge only in `releaseResources()`. `Plugin/` is unchanged, so LV2 hosts can still crash.

**Medium**
- **Audio:**
  - a `malloc` every block above 32 channels (`spatcore/io/DeviceIoCallback.h:171`);
  - `tempBuffer` allocated every block (`spatcore/wfs/InputBufferAlgorithm.h:137`);
  - `patchedInputBuffer` and `wfsOutputBuffer` grown in the callback (`MainComponent.cpp:12124, 15169`);
  - sampler vectors copied under spinlocks that the audio thread spins on (`SamplerEngine.h:55-62`);
  - output EQ coefficients torn while in use;
  - per-output rings never primed.
- **Network:**
  - a TCP send to a peer that stops reading blocks, and partial writes break the framing (`OSCConnection.cpp:383`);
  - `/remote/requestResync` has no rate limit and spawns detached threads;
  - every listener binds 0.0.0.0, the IP filter is off by default, and OSCQuery sends `ACAO: *`.
- **MCP:**
  - EQ-band and sub-tree undo writes the wrong target or silently does nothing;
  - staleness is keyed by parameter only;
  - tier 1 still includes the mute-all macro, host redirects, file paths and +24 dB EQ;
  - sampler-set nudge is off by one and never reaches audio (`MCPGeneratedToolLoader.cpp:717, 1150`);
  - a timed-out tool still runs later;
  - the OSCQuery auditor reads the ValueTree off the message thread;
  - still no token (M2).
- **State:**
  - snapshot recall skips the load validator;
  - sampler level and attenuation are unbounded (`SamplerData.h:46,138`);
  - merge-appended children skip the validator;
  - Reload Complete Backup still pairs backups by index;
  - a complete load is not transactional.
- **GUI:**
  - a failed webcam tracker is retried every 500 ms;
  - the whole window repaints at 20 Hz;
  - opening a `.wfs` file from the OS replaces the show without asking;
  - the cluster LFO runs on its own GUI timer.

**Low:**
- `cleanupBackups` is never called;
- sampler has no sample-rate conversion, and no cap on import length;
- the snapshot scope is keyed by slot;
- formal races on `processingEnabled` and `channelActive`;
- the sampler isn't prepared for channels added at runtime;
- the Low network, packaging and licensing items from the first audit.

---

## 6. Suggested next batch

1. **F1 + F2**, both one-liners with big show impact: the angle wrap and the cone early-out.
2. **R1 + B1 + B2:** finish the value-gate work, with bounds entries for the remaining OSC-writable parameters and numeric text coerced where no range exists.
3. **M3**, which also removes R4, and **P1**, the plugin destructor.
4. **N3 / N4 / N5**, the three remaining High network threading bugs.
5. **R3 + R5 + the snapshot-store backup gap**, to finish the save-integrity work.
6. **R2**, binaural prepared outside the silence scope.
