# WFS-DIY codebase audit

Branch `effects/reverb-models` @ `b7f82b1` (plus the build fix `e1916c88`), spatcore @ `5803830`, JUCE 9.0.2. Date: 2026-09-28.

## How this was done

- **Six parallel read-only reviews**, one per area:
  - real-time audio / DSP
  - network surface (OSC, TCP, OSCQuery, tracking, update checker, plugin bridge)
  - the MCP server
  - state, undo, snapshots and persistence
  - GUI, lifetimes and hardware controllers
  - build, CI, dependencies, the plugin suite and licensing

  The spatcore code these areas call into was followed too.
- **Linux Debug build** with `-Wall -Wextra -Wshadow`.
- **Tests and validation gates:**
  - the spatcore unit-test suite (`spatcore-tests: all tests passed`)
  - the repo's validation gates (kernel hashes, the spatcore dependency lint, experiment paths: all pass)
  - regeneration of the MCP tool JSON (byte-identical to what is committed)
- **Hand checks:** I re-checked every Critical and High finding in the code myself; they are marked ✔ below. One of them I reproduced with a test program built under AddressSanitizer: the OSC bundle crash.

Severity is judged for live-show use. A crash, a sudden full-scale output, or a corrupted show file during a performance ranks highest.

---

## 0. Fixed during the audit

| | Where | Issue |
|---|---|---|
| ✔ | `Source/Network/OSCMessageRouter.cpp:2021` | **The branch did not compile with GCC or Clang.** A `?:` mixed `juce::String` and `juce::var`, which is ambiguous; MSVC accepts it. So Linux and macOS builds of this branch were broken, and CI didn't notice because `ci.yml` only runs for `main` and PRs into it. Fixed in `e1916c88` (both branches are now `juce::var`), and the app then builds and links on Linux. |

---

## 1. Fix first (Critical / High)

### Crashes or unsafe output from the network

| # | Sev | Where | Issue | Fix |
|---|---|---|---|---|
| N1 | **Critical** ✔ (reproduced) | `spatcore/control/osc/OSCParser.h:163-183` | **One 28-byte OSC bundle crashes the app.** An element size of about `0x7FFFFFF0` makes `pos + elementSize` overflow, so the check passes, `pos` goes negative, and the next read segfaults. It reaches UDP, TCP, the ADM ports and tracking-OSC, and parsing happens *before* the IP allow-list runs. | Check `elementSize > dataSize - pos` (no addition); cap the nesting depth. |
| N2 | **Critical** ✔ | `OSCMessageRouter.cpp:1549-1552`, `:1651-1655`, `:1197`; `WFSValueTreeState.cpp:297`; `MainComponent.cpp:15292` | **A string argument bypasses the output and reverb gain bounds.** `/wfs/output/attenuation 1 "60"` is stored as a string (`valueWithinBounds` passes strings, and the store only clamps numbers), then read back as +60 dB. `"inf"` gives infinite gain. QLab cues that send every argument as a string can trigger this by accident. | Coerce or reject numeric strings on every numeric parameter; `jlimit(-92, 0)` plus an `isfinite` check where dB becomes gain. |
| N3 | High | `spatcore/control/osc/OSCTCPReceiver.cpp:209-229`, `TrackingMQTTReceiver.cpp:171-177` | **A TCP peer that closes cleanly leaves a thread spinning at 100% CPU.** A 0-byte read after the socket reports ready is never treated as end-of-stream. After 16 disconnects every TCP OSC client is refused, and MQTT never reconnects. | Treat a 0-byte read as EOF; add an idle timeout. |
| N4 | High | `OSCQueryServer.cpp:106-157` | **The OSCQuery HTTP server walks the ValueTree from four asio threads** while the message thread writes it. Any OSCQuery browser polling during a show can crash the app. | Build the tree on the message thread, or serve a snapshot it refreshes. |
| N5 | High | `TrackingOSCReceiver.cpp:38-56`, `OSCReceiverWithSenderIP.cpp:109-124` | **Tracking-OSC queues `callAsync([this…])` for every datagram, with no bound.** Changing the port or pattern while a tracker streams gives a use-after-free, and a flood makes the message queue grow without limit. | Use the bounded ingest queue or a `WeakReference`. |
| N6 | High/Med | `TrackingPSN/RTTrP/OSC/MQTTReceiver.cpp` | **Tracking positions are written with no `isfinite` check.** One NaN permanently poisons the OneEuro filter for that input (`TrackingPositionFilter.h:62`). | Reject non-finite values before `push()`; write through `setInputParameter`. |

### The MCP server (AI control)

| # | Sev | Where | Issue | Fix |
|---|---|---|---|---|
| M1 | High ✔ | `spatcore/control/mcp/MCPDispatcher.cpp:45` | **A deeply nested JSON body overflows the stack before any check runs.** About 1 MB of `[` crashes the app, whether or not AI is on, because parsing happens first. The server starts at launch. | Pre-scan the nesting depth (at most about 64); lower the body cap. |
| M2 | High ✔ | `spatcore/control/mcp/MCPTransport.cpp:20, 141-255` | **No authentication, `Access-Control-Allow-Origin: *`, and no Host or Origin check.** Any web page open on the show machine can call tools with a `text/plain` POST and replay the tier-2 confirmation token. DNS rebinding also works. | Per-launch bearer token, Host allowlist, reject foreign `Origin`, require `application/json`. |
| M3 | High | `MCPUndoEngine.cpp:656-672, 738-758` | **Undoing a channel create or delete resizes the channel count while the engine runs.** It bypasses `refuseWhileProcessing` and `channelTopologyChanged`. | Make topology records non-undoable, or route them through the guarded lifecycle path. |
| M4 | High | `tools/InputTools.h:289, 415`, `OutputTools.h:86`, `ReverbTools.h:74`, `MCPGeneratedToolLoader.cpp:690` | **`"nan"` or `"inf"` strings reach positions, attenuation and nudges** (`static_cast<float>(var)` followed by `jlimit`, which lets NaN through). | One shared finite-number argument reader. |

### Real-time audio: reconfiguring while the engine runs

| # | Sev | Where | Issue | Fix |
|---|---|---|---|---|
| A1 | **Critical** ✔ | `MainComponent.cpp:12826-12880` → `resizeRoutingMatrices` (11134) | **Reloading or importing a config with a different channel count reallocates the live routing matrices while processing is ON.** Only an effects-layout change stops the engine, so the worker threads hold dangling `delayTimesMs.data()` pointers. `refuseWhileProcessing` exists only in the MCP tools. | Call `stopProcessingForConfigurationChange()` when `countsChanged`, or refuse the reload. |
| A2 | **Critical** | `MainComponent.cpp:12257-12266`, `BinauralProcessor.h:76-165` | **The binaural processor is re-prepared, freeing its buffers, while it is still enabled.** With processing OFF and binaural ON, the callback still runs the binaural branch, so changing the input count crashes the app. | `setEnabled(false)` before re-preparing (the order the `timerCallback` path already uses). |
| A3 | High | `MainComponent.cpp:12137-12152` vs 14666 | **Channel counts are updated before the engine is gated**, so the smoothing loop writes past the old vectors (heap corruption). | Stop the engine first. |
| A4 | High | `MainComponent.cpp:11168-11233`, 14260 | **Engine stop and start have no handshake with the audio callback.** Containers are cleared right after the flag flips; for example, changing the GPU depth while processing. | Hold `getAudioCallbackLock()` across the flag flip and container swap, or publish new containers atomically. |

### Persistence and file safety

| # | Sev | Where | Issue | Fix |
|---|---|---|---|---|
| S1 | High ✔ | `OSCManager.cpp:2332` → `MainComponent.cpp:2624` → `WFSFileManager.cpp:1926` (also 1311, 2046, 2140, 2159, 2185) | **Snapshot names are used unchecked as file paths.** `/wfs/input/snapshot/store "../../system"` overwrites `system.xml`, and an absolute name writes an XML file anywhere. | Require `File::createLegalFileName(n) == n` and `isAChildOf(snapshotsFolder)`. |
| S2 | High ✔ | `spatcore/control/state/XmlPersistence.cpp:44` (JUCE `replaceWithText`) | **A failed write still replaces the file and reports success.** `appendText`'s result is ignored, so on a full disk or a pulled USB stick the project file is swapped for a truncated one. `createBackup`'s result is ignored too. | Write through `FileOutputStream`, check its status, then rename; fail the save if the backup fails. |

### Lifetimes and the plugin

| # | Sev | Where | Issue | Fix |
|---|---|---|---|---|
| G1 | High ✔ | `MainComponent.h:215` vs `:325`; `InputsTab.h:443`, `OutputsTab.h:313` | **`parameters` is destroyed before the tabs**, so their destructors assign into dead `std::function`s on every quit (undefined behaviour; can abort on libc++). | Call `tabbedComponent.clearTabs(); mapTab.reset();` early in `~MainComponent`. |
| P1 | High ✔ | `Plugin/Source/Track/TrackProcessor.cpp:138`, `Master/MasterProcessor.cpp:145` | **The plugins unregister from the bridge only in `releaseResources()`,** which the LV2 wrapper never calls. Removing a Track or Master leaves a dangling callback, which crashes the DAW. | Unregister in the destructor; make unregister wait for in-flight callbacks. |
| C1 | High | `.github/workflows/release.yml:58, 71, 102`; `plugins-release.yml` | **The code-signing certificate and notary key are exposed to tag-pinned third-party actions** (`import-codesign-certs@v6`, `action-gh-release@v3`, `cuda-toolkit`) and to `brew install` in the same job. | Pin actions by SHA; sign in a minimal job; delete the keychain in an `always()` step. |

---

## 2. Medium

**Audio / DSP**
- `DeviceIoCallback.h:171` builds a `juce::AudioBuffer` every block. Above 32 channels (every WFS rig) that is a `malloc` on the audio thread, contradicting the class comment. `InputBufferAlgorithm.h:137` allocates a temporary buffer every block, and `wfsOutputBuffer` is first sized inside the callback (14692).
- The binaural worker keeps using ring pointers after `stopProcessingForConfigurationChange` destroys the rings (`BinauralProcessor.h:349`).
- The per-output SPSC rings are never primed. A late worker can leave some speakers (or one ear) a block behind for good, about 1.8 m of path skew at 256 samples / 48 kHz.
- Output EQ coefficients are plain floats rewritten while the callback runs, so the audio thread can use a half-updated set (`OutputEQBiquadFilter.h:52-71`). Use the existing `RtTripleBuffer`.

**Network**
- Nested bundles cost quadratic CPU with no depth cap (`OSCParser.h:170`, `OSCManager.cpp:1985`).
- A TCP OSC target that stops reading blocks the message thread in `::send`, and a partial write breaks the framing for everything after it (`OSCConnection.cpp:362-385`).
- `/remote/requestResync` has no rate limit; each request spawns a detached thread with raw `this` (`OSCManager.cpp:1817, 5055`).
- Every listener binds 0.0.0.0. The IP filter is off by default and runs after parsing. OSCQuery sends `ACAO: *`, so any web page can read the show state.

**MCP**
- Undo writes the wrong EQ band (escape-hatch and pre-EQ records), and sub-tree records (sampler, network, ADM) report "Undid" without changing anything (`MCPUndoEngine.cpp:61-66, 668`).
- Staleness tracking is keyed by parameter name only, not channel (`MCPUndoEngine.cpp:137`), so undo can overwrite an operator edit.
- Tier 1 (no confirmation) includes +24 dB EQ boosts, the mute-all macro, redirecting OSC or MQTT hosts, and sampler file paths. Nothing is rate-limited.
- The sampler-set nudge is off by one, and its changes never reach the audio (`MCPGeneratedToolLoader.cpp:712, 1145`).
- A tool that times out still runs later, with no undo record (`MCPDispatcher.cpp:584`).
- The OSCQuery auditor walks the ValueTree on a background thread (`MCPOSCQueryAuditor.cpp:222`).
- The file-path tools accept absolute, `..` and UNC paths (on Windows a UNC path leaks the NTLM hash).

**State and persistence**
- Snapshot recall writes raw values, skipping the NaN and range gate that project loads use (`WFSFileManager.cpp:2876-2960`, `EffectsSnapshotScope.h`).
- Sampler level and attenuation are never bounded (`SamplerData.h:40-47`), so a file with `samplerSetLevel="40"` gives +40 dB.
- Merge-appended children skip the load validator (`XmlPersistence.cpp:225`).
- "Reload Complete Backup" pairs backups from different save generations by index (`WFSFileManager.cpp:527-613`).
- Loading a complete config is not transactional: one corrupt file leaves a half-loaded session, which the next autosave then persists (`WFSFileManager.cpp:419-525`).

**GUI and controllers**
- A failed webcam head-tracker selection retries every 500 ms forever on the message thread (`MainComponent.cpp:15833`).
- The whole window repaints at 20 Hz for a CPU display that no longer exists (`MainComponent.cpp:16497`).
- Opening a `.wfs` file from the OS replaces a running show without confirmation (`Main.cpp:198`).
- The cluster LFO runs on `ClustersTab`'s own timer, which beats against MainComponent's control tick.

**Build, CI and supply chain**
- Manual (`workflow_dispatch`) releases build the selected branch, not `inputs.tag`: there is no `ref:` on checkout.
- `wfs_hip.dll` is shipped as a committed prebuilt binary with no hash or provenance. The OpenCV installer `.exe` is downloaded and run without a hash check.
- CI builds only Debug, with no `-Werror`, no sanitizers and no app or plugin tests. Nothing checks that generated files are fresh. **CI does not build feature branches at all**, which is how the compile break in §0 got through.
- The SOFA files that libmysofa parses can come from users or shared projects, and there is no fuzz target for them.
- The installers ship without the AGPLv3 text (JUCE) or the full BSD and Apache texts.

## 3. Low (selected)

- **Sampler:** no sample-rate conversion, so 44.1 kHz material plays about 8.8% sharp on a 48 kHz rig. Import lengths have no cap.
- **Reverb upsampling:** when the block size isn't a multiple of the ratio, the last samples of a block are stale.
- **Formal data races:** plain `bool`/`int` shared with the audio thread (`processingEnabled`, `numRenderSources`, `std::vector<bool> channelActive`).
- **Backups:** `cleanupBackups` is never called, backup names only have 1-second resolution, and snapshot backups can match the config-backup patterns.
- **Snapshots:** the scope for new snapshots is keyed by slot and not remapped after a channel is deleted.
- **Bounds drift:** `eqQ` and `eqSlope` maxima in the CSV and MCP tools vs `WFSParameterDefaults.h`, and `binauralOutputChannel`. Patch `cols` has no bound.
- **Network, minor:**
  - the RTTrP decoder can step backwards;
  - an oversized MQTT packet desyncs the stream;
  - a null-dereference window in `SimpleWebSocketServer::sendTo`;
  - the Find My Remote password is logged in clear;
  - the update-banner URL is opened without a host check.
- **Dead OpenSSL 1.1.1g:** the libraries are still on every link line, and the module header defaults to secure mode on.
- **Linux packaging:**
  - `install.sh` upgrades nest `lang/lang`;
  - system mode writes the `.desktop` file where desktops don't look;
  - two bridge `.so` copies split the plugin registry;
  - an unneeded touchscreen udev rule.
- **Windows crash handler:** it logs (allocating) before closing ASIO.
- **Lambdas:** two capture transient UI raw (`SetAllInputsWindow.h:882`, `GettingStartedWizard.h:517`).
- **Encoding:** a double-encoded `Â°` in `UsbDongleClient.h:259`.
- **Compiler warnings in app code:** a handful, all benign: misleading indentation in `ReverbAutoLayoutTool.h:255-257, 619-620`, `-Wmissing-field-initializers` (WizardStep, PendingParamUpdate), and unused variables (`MainComponent.cpp:15663`, `NetworkTab.h:5134`).

## 4. Maintainability

1. **`MainComponent.cpp` is 17.7k lines.** About 7,500 of them (lines 3239-10760) are env-var-gated self-tests compiled into the release binary. The constructor is about 2,970 lines and `timerCallback` about 1,400 lines at 200 Hz.
   - Move the self-tests to their own translation unit.
   - Split the control-rate engine (LFOs, speed limiter, calculation engine) out of the GUI tick. It would also keep source motion running when the UI stalls.
2. **Header-only tabs** of 5-8.6k lines, all compiled into one TU (which needs `/bigobj`). Moving each tab's body into a `.cpp` would help incremental builds a lot.
3. **Copy-pasted code:** `CircuitTabHandler` appears 4 times, and the colour-scheme editor lambda and the help-text/mouse-listener block appear in about 8 tabs. Factor them out like `EffectsFieldEditing.h`.
4. **Dead code:** three `#if 0` blocks (241 lines) in `OutputsTab.h`, and a few stale localization keys in the full-tier language files.
5. **Positives:** only 4 TODO/FIXME comments in `Source/`, and no warning suppressions. Placeholders agree across all 9 languages.

## 5. What is solid

- **Lock-free foundations:** `RtTripleBuffer`, the SPSC rings, `LevelMeteringManager` and the device-I/O block clamp are correct. The GPU algorithms and `EffectsEngineCore` use try-lock plus a ready flag, and that is the pattern the CPU path should copy.
- **Entry-point validation:** NaN/Inf and range checks at the main OSC float path and at project load. Channel IDs resolve through permanent numbers, and I found no out-of-range index writes.
- **Bounded queues and logs:** the OSC ingest queues (4096 coalesced / 256 FIFO), the tracking queue (1024) and the logger (1000 entries).
- **Saving:** temp-file plus rename under normal conditions, and autosave cannot overwrite a project that was never loaded.
- **Undo:** clears on import, and structural edits are not undoable.
- **MCP generic tools:** careful validation, a registry whitelist, a batch cap of 100, and AI is off at launch.
- **Lifetimes:** alive tokens on the update checker, USB dongle, controllers and SOFA loader, balanced listener add/remove, and no modal loops.
- **Release plumbing:** hardened-runtime re-sign, protected signing environment, no `pull_request_target`, submodules pinned, and kernel-hash and dependency lints.
- **Localization:** consistent across all 9 languages.

## 6. Suggested order of work

1. **Network hardening, one small PR:**
   - N1: the parser overflow (a one-line fix plus a regression test using the 28-byte packet);
   - N2: string gain;
   - N6 and M4: finite-number checks;
   - S1: snapshot-name sanitising.
2. **Reconfiguration safety (A1-A4):** a single rule, "no structural change while the callback can see it", enforced in one place with the audio callback lock or an atomic publish.
3. **MCP transport (M1, M2):** a token and Host/Origin checks, and a JSON depth pre-scan.
4. **Save integrity (S2)** and **shutdown order (G1)**, both small.
5. **CI:**
   - build every branch, or at least `effects/**`, on GCC or Clang;
   - pin actions by SHA;
   - add an ASan job running the spatcore tests plus an OSC-parser fuzz target;
   - add `ref:` to the release checkouts.
