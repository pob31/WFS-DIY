# WFS-DIY re-audit (2026-09-29)

> **For the agent picking this up:** every file:line below refers to branch **`effects/reverb-models` @ `4c94e78f`** (spatcore submodule @ `9384e35`), not to `main`. Check that branch out, with `git submodule update --init` so spatcore is at that pin, before working through it. The first audit this compares against is `Documentation/WFS-DIY-audit_20260928.md` on that same branch. Items marked ✔ were checked in the code or reproduced; the others come from review agents and should be confirmed before fixing. CI changes were left out on purpose.

> **Status:** section 7 records what a second pass confirmed, what it added, and what has been fixed since.

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

---

## 7. Status after the local pass (2026-09-29)

Every item in sections 2–5 was re-read in the code on `effects/reverb-models` (Windows dev box) by three review agents and me. Nothing was refuted. Below: what is fixed, then what the second look added to each item. "Confirmed" means as described above; only the differences are written out.

### 7.1 Fixed

| Item | Commit | Notes |
|---|---|---|
| F1 | `724e8407` | `std::remainder` in all three wraps; a non-finite angle reads as 0. Reproduced first: the pre-fix build hung on `/wfs/input/positionTheta 1 1e10` and ignored the close. |
| F2 | `6ab4fd08` | The shortcut fires at `angleOn >= 180` only, which is exact (the audit's `180 − angleOff` would still skip the mute zone past angleOn when the two cones overlap). **Show-visible:** a speaker or feed with Angle On ≥ 90 now mutes its front cone. |
| F3 | `73c669ab` | Orientation delta and each member's result are wrapped; other linked parameters keep the clamp. |
| R1, B2 | `ae80f759` | Bounds for `gmLayer0/1/2Enabled`, `inputMuteReverbSends`, `outputApplyToArray`, input and reverb `muteMacro` (0..25, per the UI CSVs). `inputSamplerActiveSet` stays unbound on purpose (OSC is 1-based, the store 0-based); its handler now range-checks the set number. Side effect: these parameters are typed `i` in OSCQuery and the tablet echo after a load. |
| B1 | `e944dd2b` | Out-of-range or non-finite values are refused with a session-log line; the write goes through `WFSValueTreeState::setClusterLFOParameter` (store gate, phase wrap). |
| F12 | `619ea3b0` | Also corrected the angular-attenuation pseudo-code next to it (rear-axis signs, mute edge at `180 − angleOff`). |
| Tests | `eb51b7a5` | `WFS_TEST_VALUE_GATES` G5–G9 (75 checks; G6 reads the level matrix) and new `osc_replay.py` checks. The pre-fix exe fails all six new OSC checks; a mutant with the fixes reverted fails all 17 targeted self-test checks. All 8 replay drivers pass on Debug; self-test and `osc_replay` pass on Release. |
| M3 (+R4) | `c82ea3ae` | Channel create/delete/recount records (hand-written and generated tools) are filed non-undoable, like `session_save`; the tool descriptions say to use the opposite tool. `mcp_replay.py` checks it; the pre-fix exe fails all three checks (the undo took the count back to 8 in the tree alone). |
| P1 | `c13a4b53` | Both plugin destructors unregister first, and every bridge registration has a call gate: a call runs holding it, unregister closes it and waits for calls in flight (re-entry and self-retirement handled). ABI unchanged. A standalone harness built from `Bridge.cpp` passes 10/10; the previous bridge fails 5. Bridge, Master and Cartesian Track VST3 build on Windows; macOS/Linux left to plugin CI. |
| N3 | spatcore `f82b1e5` (branch `fix/reaudit-network`, PR pending) + `3ac3aa81` | A 0-byte read after the socket reported readable is end of stream (TCP handler ends); MQTT leaves its read loop on -1 or an empty readable socket and reconnects. |
| N5 | `717df05d` | Tracking OSC gets its own bounded `OSCIngestQueue` (FIFO only, 5 ms drain, drops logged); `stop()` joins the socket thread before destroying the queue. |
| N4 | `4489ec27` | HTTP requests are queued and answered in one pass on the message thread (one tree per batch); no HTTP thread waits, and only the queue holds a response. **Found on the way:** a first version that blocked the asio threads crashed `midi_snapshot_check` 3/3 in WinINet: every snapshot recall re-ran the MCP OSCQuery audit, the replaced auditor was stopped with a 3 s `stopThread` while its 750 kB reply could not be sent, and JUCE killed it inside WinINet. Also fixed: the audit runs only when OSCQuery starts, and the auditor's fetch is cancellable. |
| Tests | `953d208f` | New `network_threads_check.py` (N3 TCP + MQTT with a fake broker, N4 GET storm smoke check, N5 burst). The pre-fix exe fails N3 twice and N5. Full sweep (self-test + 9 drivers) passes on Debug; self-test, `osc_replay`, `mcp_replay`, `midi_snapshot_check`, `network_threads_check` pass on Release. |
| R3, R5, S2, R7, R8, R6, B5 | `79088fe2` + spatcore `ff3f99b` (branch `fix/reaudit-save-integrity`, PR pending) | Complete save backs up all six files first and removes its own backups if one fails. Backups get millisecond names and never replace an existing one. Snapshot and template saves back up themselves, into `backups/snapshots` and `backups/templates`. Write failures are logged in Release, and both autosaves report failure (the patch save retries every minute). Store with QLab stops on a failed save. Save temp files are left out of the lists. `./`-joined names accept a leading `~`. Windows-reserved names are refused. The sampler export checks its write. `cleanupBackups` is still not wired up: that needs a retention decision. |
| Tests | `365c5238` | 93 self-test checks (new G4: reserved names, snapshot/template backups, temp files); `session_roundtrip.py` locks `inputs.xml` for an R3 case, which the pre-fix exe fails seven ways; spatcore-tests gain the R5 cases (a mutant with the old naming loses the older backup). Full sweep on Debug; self-test, round trip, `osc_replay`, `midi_snapshot_check` on Release. |

### 7.2 What the second look added

**Save integrity**
- **S2:** confirmed as written. Also: `setExtendedSnapshotScope` writes without a backup (no caller today); the OSC store's "no project folder" branch is DBG-only; after a failed patch autosave the countdown is not re-armed, so the edit exists only in memory and is lost at exit; no save chooser passes `warnAboutOverwriting`, so on Windows exports overwrite without a prompt.
- **R3:** confirmed. It also breaks Reload Complete Backup: the skipped section has one backup fewer, so pairing by index mixes generations there as well. The error text ("the file on disk is unchanged") reads as if nothing was saved.
- **R5: worse than written.** `File::copyFileTo` deletes an existing target first, so a second backup of the same file in the same second **silently replaces** the first, which destroys the only copy of the generation before the first save (Save then quit, an autosave next to a Save, two MCP saves). The lock case (Dropbox/antivirus) exists on Windows only and then feeds R3.
- **R6:** cause: on POSIX `File::isAbsolutePath` treats a leading `~` as absolute, and `~user` expansion looks up the user "home.xml". Fix: `getChildFile ("./" + name + ext)`, keep the parent check.
- **R7:** confirmed; also `scopes[name]` is set before the save, so after a failure the cache disagrees with the disk.
- **R8: worse than written.** The leftover temp also takes the snapshot's MIDI trigger note ('.' sorts first, first in name order wins), and a truncated temp still yields a binding. `ignoreHiddenFiles` filters nothing on Windows (hidden is an attribute there) and would hide legitimate dot-names on POSIX: skip `.*_temp<hex>.xml` explicitly in the four scans instead.
- **New (Medium): the backups folder is one namespace.** Snapshot and template backups are `<name>_<timestamp>.xml` next to the section backups, which are listed by the glob `<prefix>_*.*`. A snapshot named "inputs…" appears in `getBackups("inputs")`, `importInputConfig` accepts it (a snapshot has an `<Inputs>` child), and Reload Input Backup can load a snapshot as inputs.xml. Fix: `backups/snapshots` and `backups/templates`. Wiring up `cleanupBackups` before this would delete snapshot backups.
- Backup modification times differ by platform (Windows/macOS copies keep the source's, Linux's don't), so "newest backup" differs.

**Engine and MCP**
- **R2:** the allocation is about **166 MB** at 64 sources / 96 kHz, not 49: `hrtfEngine.prepare` prepares both the structural and the SOFA renderer whatever the mode (about 309 MB at the 136-source budget). Estimated 15–100 ms of whole-PA silence. The callback only touches the binaural buffers while `isEnabled()`, so a one-shot barrier on the audio callback lock (no counter) is enough; swapping in a new object is worse (non-atomic pointer, published SOFA set, registrations).
- **R4 / M3:** confirmed. Also: undoing a delete appends a new default **mono** channel (parameters lost, a stereo channel comes back mono), and undoing a count-tool record writes `mono`/`stereo` ids that are not parameters. The undo is reachable from the MCP tools (tier 1), the history window, the overlay ×, and the keyboard shortcut. Smallest fix: `record->undoable = false` in `create()`, `del()` and `setCounts()`; the engine already skips or refuses such records.

**Plugins**
- **P1:** confirmed. Also: dispatch copies the registry entry and calls it after releasing the lock, so unregistering in the destructor is not enough on its own: a dispatch racing the destructor still reaches freed memory. A dead Master also blocks any new Master from registering.
- **B3:** partly: the recursion runs on the plugin's own "JUCE OSC server" thread (stack 1 MB Windows, 512 KB macOS), not the host's audio thread, and every level deep-copies its subtree twice (O(depth²)). The parse result is also destroyed recursively on the host's message thread. Only a replacement receiver fixes it (JUCE's parser is internal).

**Network**
- **N3:** confirmed. Also: the spinning TCP handler never goes inactive, so after 16 such peers every new client is refused.
- **N5:** confirmed, plus a **use-after-free**: `TrackingOSCReceiver` is the only receiver that never calls `setRawDataCallback`, and `stop()` frees the receiver while queued lambdas still hold its raw `this` (port change, tracking off, shutdown).
- **B4:** confirmed. Also: the IR is opened on the message thread inside `timerCallback`, so an unreachable UNC host freezes the UI for the SMB timeout; no IR length cap.
- **B5:** partly: on Windows 11 `NUL.xml` is an ordinary name; the risk is Windows 10 and older, and projects moved there. `<>"|?*` and control characters fail late with a misleading "failed to write" message.
- **B6:** confirmed; the read before the parse is unbounded too.

**Sampler, LFO and GUI**
- **F4:** confirmed, and the map hides it (the marker shows the offset only while playing). The override is only ever cleared by the Lightpad and tablet release paths; `releaseChannel` also writes the audio thread's atomics from the message thread.
- **F5:** confirmed; NoteOn and NoteOff in one block give a burst of the whole fade-out at full level.
- **F6:** confirmed, plus: switching between two non-Off shapes also jumps (no crossfade), and the cluster LFO's rotation and scale jump to half their amplitude when set to Off. No slew downstream: a 3 m jump is an 8.7 ms delay step in 20 ms (a zip), above about 10 m a 10 ms dropout.
- **F7:** confirmed; an absolute polar target of start ± 360 is refused the same way, and the effect returns share the code.
- **F8:** confirmed, plus a second failure mode: editors restricted to `-0123456789.` silently drop the comma, so "-2,5" becomes **−25 m**. Same pattern in the tracking offset/scale (a cleared scale collapses tracked sources to the origin), ADM mapping fields, ports (a cleared port is 0), the distance constraint and the whole array helper. Editing one axis also re-saves the other two from their rounded display text.
- **F9:** confirmed. **F10:** confirmed; the NaN reaches the speakers only on the GPU renderers (the CPU path drops that speaker, the one closest to the source). **F11:** partly: the cited `SamplerFileOps::importSampleToProject` has no callers; the live copy in `SamplerSubTab.h` has the same loop and also ignores the copy result.
- Section 5, sampler bounds: confirmed; every sampler numeric lacks a bounds entry (a NaN pressure curve reaches the audio). Sampler-set nudge: confirmed; the tab does not refresh either.

**Harness:** `osc_replay.py` does not back up `WFS-DIY.settings`; a forced kill leaves `cleanShutdown=0` and `lastProjectFolder` on the temp fixture.

### 7.3 Order from here

Done: M3 (+R4), P1, N3, N4, N5; the save-integrity group. Next: R2; then B3, B4, B6, F4–F11 and the section 5 items. Progress is recorded in 7.1.
