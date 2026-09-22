# Effects channels — implementation status and handoff

**As of 2026-09-23, after Phase 7.** Branch `effects/phase-7` (off `effects/phase-6`), submodules
clean at their pins (`spatcore` v0.3.3 = `4523d1b`). Phase 7 added ten app commits and no spatcore
commit (§10). Phase 6 added fourteen app commits and one spatcore commit (the schema-free
send-matrix widget and the per-slot meter read, PR #14).

An effect channel is now a render source of the show: its chain runs on the engine's own realtime
thread, its return is popped into a render-source row of every block, the calculation engine
computes the source-by-effect feed matrix and the return rows of the output and reverb matrices,
AutomOtion can move a return by an offset, and the engine's meters are readable from the
application, and since Phase 6 an operator can see and edit all of it from the Effects tab, the
Stream Deck and the four tab-level OSC verbs (§9), and since Phase 7 every snapshot, MIDI cue and
QLab export carries the effects too (§10). What is still missing is the OUTBOUND half of the control
surface (Phase 4's C8 echo and OSCQuery node, the C9/C10 tool entries).

The design is `Documentation/effects-channels-plan.md`. Read its **§12.4 through §12.7** first —
those are the correction logs, and essentially every line reference in §6 and §7 of the body is
stale. Where the document and the code disagree, the code is right.

---

## 1. What is done

**spatcore, all merged:** Phase 1 (primitives + module contract, v0.3.0), Phase 2 (all nine
module types, v0.3.1), Phase 3 (the engine, loop guard, render-source map, v0.3.2).

**App Phase 4 — the data model is COMPLETE.** Eleven commits, `ed93f38..434cddb`:

| Commit | What |
|---|---|
| `ed93f38` | render gate for the engine + spatcore pin to v0.3.2 |
| `b34b237` | `backfillFromTemplate` fixed — it had drifted from the `mergeTreeRecursive` it claims to mirror |
| `f9fbf93` | the parameter surface: 192 identifiers, 418 constants, band/tap tables |
| `aad59d3` | the channel family: `<Effects count="0">`, the 20-node builder, accessors, scope routing, position latch |
| `76d5afa` | three live reverb pre-EQ OSC defects (pre-work, so the effects parser is modelled on working code) |
| `7863e85` | persistence: `effects.xml`, the Config properties, the eviction hook |
| `c4db037` | the load path had no template to merge onto |
| `3052c00` | what the phase-wide audit found (outbound band form, the two mirror static_asserts) |
| `5c3bb18` | the eviction hook inferred retirement from absence |
| `9f60833` | the parameter CSV + design corrections R5-1..R5-8 |
| `434cddb` | the send matrix, its guards and its lifecycle |

**Gate state:** Release x64 clean; structural self-test 600 assertions / 0 fail; all seven
control-replay drivers pass with every golden unchanged.

---

## 2. What is NOT done: the control surface

**Phase 4's control surface is half written.** Measured, not assumed (the two UNCOMMITTED lines
are the working tree, not `HEAD`):

```
OSC effect addresses in Source/Network : 164 in getEffectAddressMap
                                         + 10 in getConfigAddressMap  (C5/C6, UNCOMMITTED)
effect entries in OSCParameterBounds   : 164 (C5/C6, UNCOMMITTED; effectChannels and the six
                                         packed ROW identifiers are deliberately absent)
effect tools in generated_tools.json   : 0
WFS-UI_effects.csv registered          : 0 of the 2 lists that read it
```

**The INBOUND half is written and gated but not yet committed** (C5 + C6: the address map, the
eight-shape parser, the bounds entries, the dispatch through the typed accessors, the
routing/suppression entry, the config globals with `UndoDomain::Effects`, the cell ingest keying
and the ramp argument parsed-but-not-applied). What is still missing from C5's own list: the
polar aliases and the snapshot/clear verbs, which are **recognised and refused with a reason**
rather than mis-parsed. Everything OUTBOUND is untouched: nothing echoes an effect change and
OSCQuery publishes no `/wfs/effect` node, so the replay reads effects out of the saved
`effects.xml` instead.

An effects channel exists, persists and maintains itself correctly, **and nothing outside the
application can read it back.** (Phase 6 made it visible and editable from INSIDE the application
and from a Stream Deck, and receives four inbound verbs; the outbound half is unchanged - see §9.)

### THE HANDOFF WARNING — RESOLVED, AND IT WAS WRONG

The warning said nothing could create an effect channel through any shipping surface, and that
Phase 5's audio check therefore could not be performed. **That was already false when it was
written.** `WFSFileManager::applyConfigSection` calls `setNumEffectChannels` from
`<IO effectChannels="N">` and `applyEffectsSection` re-syncs, so **opening a project whose
system.xml says `effectChannels="2"` builds two live channels**, with no UI, no OSC and no env
hook. That is the route Phase 5's audio check used, and it needed nothing from Phase 4.

The rest of §2 stands: an operator still cannot SEE a channel, because inbound OSC is not a
control surface on its own - nothing reads back. Phase 4's outbound half (C8) and tool entries
(C9/C10) are the work that finishes it.

---

## 3. Phase 4 remainder — four commits, with the decisions already made

Lettering follows the audit at
`<scratchpad>/phase4-spec/osc-and-oscquery.json`, whose `commitBreakdown` is the detailed plan.
C0–C4 and C9's CSV are done.

**C5 + C6 — OSC inbound. WRITTEN, GATED, UNCOMMITTED.** Address map, `isEffectAddress`,
`getEffectParamId`, `getEffectParamKind` (the eight-shape classification table, consulted BEFORE
any argument is read), `parseEffectMessage`, `isEffectParamRampCapable`, bounds entries (including
the four cell pseudo-identifiers and excluding the six row identifiers), the dispatch branch
through the typed accessors, `/wfs/effect/` added to the routing/suppression test, the cell
bypass list and the sub-indexed coalesce key.

> **The channel count is the first one any protocol can address, and it is STOPPED-ONLY.**
> `/wfs/config/effectChannels` is accepted while processing is stopped and REFUSED with a logged
> reason while it runs: an accepted count write reaches `handleChannelCountChange`, which stops
> processing to rebuild the shared rings, so without the guard one mistyped cue address would stop
> a live show. Same rule and same words as `MCPGeneratedToolLoader`'s channel-count gate. Both
> halves are gated in `osc_replay.py`, the running half through a second app run started with
> `WFS_TEST_AUTOSTART_PROCESSING` (a project can never LOAD with DSP flagged running, by design).
> The guard is read TWICE: once on the ingest thread, and again inside the `callAsync` where the
> write actually happens, because an operator who pressed Start between the two would otherwise
> have had a count write land on a running engine — the one thing the rule exists to prevent.
> Mutation-testing shows the redundancy is real: removing only the first check leaves the replay
> passing, and only removing both lets the count move under a running engine.

> Snapshot, clear, selected and editOnMap are recognised as VERBS and refused with a reason naming
> the phase they land in — not silently mis-parsed as unknown parameters. Polar aliases are not
> written.

> **THE TYPING GATE, AND WHY IT IS NOT ONLY ON `/wfs/effect/`.** "A non-numeric string at a
> numeric effect parameter is refused" was true of the 164 addresses parsed by
> `parseEffectMessage` and false of the ten this commit added to `getConfigAddressMap`, because
> those go through `parseConfigMessage` — which stored any string verbatim, and whose range gate
> waves strings through by design. `/wfs/config/effectChannels "seven"` therefore read as
> `static_cast<int>` of a String == **0** and DELETED EVERY EFFECT CHANNEL: silently, reported as
> a success, and not undoable (`setNumEffectChannels` is a structural edit). `parseConfigMessage`
> now applies the same rule as its effects counterpart — two config parameters name things
> (`effectsGlobalLinkNames`, `effectsGlobalFeedGpuDevice`) and may carry text, the other 46 are
> numbers, a numeric string still coerces so QLab's string-typed arguments keep working, and an
> int-typed parameter is stored as an int so it does not read back as a default after a load.
> `effectChannels` additionally has to be a WHOLE number in range, because it is the one config
> parameter with no bounds entry and `setNumEffectChannels` would otherwise answer "set 40" with
> a silent clamp to 32.

> **A MESSAGE IS NEVER LOST, AND THE REASON REACHES THE SESSION LOG.** Three refusals used to
> return with an EMPTY reason (an unknown parameter name, a message with no arguments, a missing
> value), and the dispatch only logs when a reason is present — so a misspelled address was
> indistinguishable from an unplugged cable. A write to a channel that does not exist was worse:
> the parser bounds the effect id against `maxEffectChannels`, not the live count, so it parsed as
> valid and then evaporated in five of the six per-channel shapes (`setEffectParameter` returns
> void on an invalid tree; the instanced, band and tap arms were `if (node.isValid())` with no
> else). That check cannot live in the parser, which has no session to ask, so it is one test at
> the head of `applyEffectUpdate`, on the message thread, covering all six shapes.
>
> An argument the shape cannot spend is now refused rather than swallowed: the trailing number in
> `/wfs/effect/minimalLatency <id> 0 9` is not a transition time (that parameter is not
> fade-capable), so either it is one argument too many or the address is the wrong one — and under
> the second reading the "value" already read is an INDEX. Six shapes share this prefix, so
> picking the wrong one is the likeliest client error in the family; the arity is the only thing
> that can catch it, and counting is free. For the 98 `isEffectParamRampCapable` rows the
> `<value> [fade]` ambiguity is irreducible protocol design, exactly as in the input family.
>
> Finally, **`logRejected` is not a log.** `OSCLogger` starts disabled and the only thing that
> enables it is a human ticking the switch in the Network Log window, so every reason above was
> discarded on any machine nobody was watching — which is every machine running a show. The
> effects and config refusals now also reach the SESSION log, the same conclusion the ingest-drop
> counter already reached, through a token bucket (20 lines of burst, 5 a second sustained) so a
> client stuck in a retry loop cannot flood it and the lines it drops are counted into the next
> one that gets through.
>
> All of this is gated in `osc_replay.py`: `TYPING_ACCEPTED` / `TYPING_REFUSALS` are sent LAST, so
> every assertion about them is "the refusal left the legitimate value alone", and
> `EXPECTED_LOG_REASONS` asserts the reasons appear in the session log — the only assertion that
> can catch a lost message, since a lost message leaves no trace in the state.

> **C6 IS ALREADY DECIDED: the app-only route.** Add the four cell addresses to
> `ingestClassifier.bypassAddresses` (OSCManager.cpp:137) and extend OSCManager's second-stage
> coalesce key (`paramId + ":" + channelIndex`) to carry the sub-index. Reason: spatcore's ingest
> key is `address + "|" + firstInt32` (`OSCIngestQueue.cpp:141`), so a 3-argument cell message
> keys on the EFFECT alone and only one cell per effect survives a drain tick — a cue setting 64
> sends would land one. `/wfs/input/mutes` is already in that bypass list for exactly this reason
> and its comment is the rationale verbatim. The alternative (extending spatcore's `CoalesceRule`)
> puts a submodule PR and a tag inside an app phase. **Cost to document at the call site:** the
> bypass list shares the 256-entry ingest FIFO, so a burst beyond that drops the excess; a surface
> changing many cells at once should send the whole ROW.
>
> Cells and rows must dispatch ONLY through the typed accessors (`§4` below), never through the
> generic parameter path.

**C7 — ramper generalisation.** Key becomes `{family, channel, paramId, subA, subB}`; the three
hard-wired `getInputParameter`/`setInputParameter` call sites become per-family functors. **Keep
the `value.isDouble()` guard at the ramp entry** — it is the only thing stopping a CSV row
entering the ramper, where `static_cast<double>` on a `juce::var` holding `"0,1,0,0"` yields 0.0
and each step would overwrite the whole row with a bare double. Gate that the existing input ramp
replay case does not move.

**C8 — outbound + OSCQuery.** `getEffectMappings`, `buildEffectMessage`, the Effect arm of the
ancestor walk, `buildEffectChannelJson`, the `/wfs/effect` container, `getReverseMap`,
`resolveOSCPath` (**use the node's `id`, not the child index — the reverb arm is already wrong
this way**), `structurePathForContainer`.

> Known wall, decide here: OSCQuery publishes ONE node for all bands of an EQ and gives it no
> VALUE, so band values are unreadable over OSCQuery today. The effects EQ and delay taps hit the
> same wall. Either publish an array VALUE or accept the gap explicitly.

**C9 rest + C10 — codegen and the tool layer.** Register `WFS-UI_effects.csv`, the config tables,
regenerate `generated_tools.json` **and** the `mcp_replay.json` census golden together (that
golden is the gate that fails otherwise, and the plan already says it is regenerated here), the
loader/registry/lifecycle/audit work, and the hand-written effect tools.

> **R5-7: there are TWO hardcoded CSV lists.** `tools/mcp/wfs_codegen_config.py`
> (`CSV_FILES_ORDER`) and `tools/audit_param_bounds.py` (`CSV_FILES`, :45-53). The second belongs
> to the bounds auditor, whose entire job is catching drift between the CSV, the defaults header
> and the bounds table. Register in only the first and it **silently** skips all 174 effect
> parameters, and the zero-drift property verified when the CSV was written stops being checked.
>
> **R5-8: only `effectChannels` moves to the config CSV.** The config layout has no OSC path
> column, so moving the map toggle there would lose the `/wfs/<family>/mapVisible` convention every
> family follows. `GLOBAL_ROWS_IN_CHANNEL_CSVS` (wfs_codegen_config.py:460) already exists for
> exactly this and already carries `reverbsMapVisible`; add the nine `effectsGlobal*` and
> `effectsMapVisible` there instead.

---

## 4. Phase 5 — audio wiring: DONE

Eight commits, each gated on a Release build, the `WFS_TEST_CHANNEL_LIST` self-test, the seven
control replays with no golden regenerated, the two offline-render canaries, the kernel hashes and
the dependency lint. Every new assertion was mutation-tested: the mechanism it names was broken,
the build repeated, the assertion seen to fail, and the source restored from a backup rather than
from git.

| Commit | What it did |
|---|---|
| render budget | `maxRenderSources = maxInputRenderSources + maxEffectChannels` = 136, asserted against spatcore's `kMaxRenderSourceSlots`. App-only, no spatcore edit, no rename. |
| count refactor | `handleChannelCountChange()` reads all four counts from `parameters` instead of taking them; nine call sites, behaviour-neutral. |
| calc engine | The engine knows an effect return is a source: return rows in the output and reverb matrices, the source-by-effect feed matrix at the effects budget's stride, kind-aware `getRenderSourcePosition`, the otomo offset, the solo masks, the feedback-cycle mask, and the listener groups that dirty them. |
| render sources | `recomputeRenderSourceCount` builds the 3-argument map, so the returns become rows of the show everywhere `numRenderSources` is read. |
| the engine runs | `Source/DSP/EffectsHost.h`: config, prepare and release, the pop into the render-source rows, the tree-to-POD cook with per-channel revisions and per-tick coalescing, and the telemetry. MainComponent wiring, ring depth, teardown order, the reload guard and `WFS_EFFECTS_TRACE`. |
| AutomOtion | `AutomOtionFamily` makes the processor family-driven; the effects family animates an offset instead of writing a position. |
| meters | The engine's per-channel peaks reach `LevelMeteringManager` (5 ms poll, max-hold, the input meter's ballistics, freshness from the batch counter), and the effect sends reach the Inputs-tab visualisation as a third group of bars. |
| docs | This section, the plan's §12.7 and the architecture thread table. |

**The audio check, which is the only way to hear the wiring.** A throwaway copy of the
control-replay fixture with `effectChannels="2"` in its system.xml, input 1 sent to effect 1 with
its first EQ live, a sampler tone held on input 1 through `/remote/pad/touch`, a real Windows Audio
endpoint injected into a backed-up copy of `WFS-DIY.settings`, and processing started by the app
itself through `WFS_TEST_AUTOSTART_PROCESSING`, because the shell this runs from has no interactive
desktop to long-press the button from. With `WFS_EFFECTS_TRACE=1` the session log then reads:

```
Effects engine prepared: 2 effects, 10 sources (returns from slot 8), block 480 @ 48000 Hz, cushion 1 block(s), workers 1
effects: batch=1236 lastUs=39.7 perWake=1 skips=0 wraps=0 clears=0 lockFail=0 workers=1 cushion=1
  fx 1 feedPk=-30.6dB retPk=-25.9dB under=0 disc=0 nan=0 lg=0/- lat=0 rev=1
  fx 2 feedPk=-120.0dB retPk=-120.0dB under=0 disc=0 nan=0 lg=0/- lat=0 rev=1
```

The batch counter advances at block rate, the return sits above the feed because the EQ band is
what the signal went through, the effect nothing is sent to stays silent, and nothing underruns or
trips the loop guard. Removing the notify to the driver freezes the batch counter at zero and
climbs the underrun count into the thousands, so the check is load-bearing rather than decorative.

The driver is `tools/validation/`-shaped but lives in the session scratchpad rather than the repo,
because it injects a real audio endpoint into the user's settings file. Phase 8 is where it becomes
a fixture.

**offline-render is a canary here, not a gate.** It compiles zero app headers, so it cannot see the
render-budget flip, the calculation engine or MainComponent. Its baseline holds 15 WFS/reverb
hashes plus 11 effects hashes; five `stereo` combos have never been baselined and report MISSING on
every run, which is pre-existing. What matters is that no combo MISMATCHes.

**What phase Y and phase O of the self-test can and cannot assert.** A return row in minimal-latency
mode is measured against its own row minimum, and the delay rule (the input's, parallax included)
makes the source-dependent term identical across the row. On a rig whose outputs share a listening
point the two cancel exactly: the row is flat at zero and stays there however far the return
travels. A moved return therefore re-LEVELS its row in both modes but only re-TIMES it in
absolute-latency mode, and the self-test says so in two separate checks rather than one that reads
like an invariant and is not.

---

## 5. Design corrections from the 2026-09-17 conversation (§12.6)

These came from the operator describing intended use. **They land in Phases 6 and 7, not 5**, but
they amend decisions the document records as *confirmed*, so do not re-derive the old ones.

| # | Correction |
|---|---|
| R5-1 | **Mute does NOT propagate through link groups.** The plan put mutes in the propagating set, so two linked channels shared one mute state and neither could be silenced alone. Move `effectMute`, `effectMutes`, `effectMuteMacro`, `effectMuteReverbSends` from `isAbsoluteOnly` to `isExcluded`. `effectSolo` was already excluded — that inconsistency is evidence the propagating half was an oversight. |
| R5-2 | **Group mute is an ACTION, not a coupling.** Independence and group shortcuts are only compatible if the shortcut WRITES every member once and leaves each independently editable. Propagation cannot express it: under it, unmuting one member unmutes all. |
| R5-3 | The bunch may need no new membership — with R5-1 applied, `effectLinkGroup` no longer couples mute state, so the same membership can address a group-mute action. If bunches need membership independent of parameter linking, that is a second identifier and must be decided before the GUI. |
| R5-4 | **The third matrix level was missing its trim.** Add `effectArrayAtten1..10` on `<Return>`, mirroring `inputArrayAtten1..10` (-60..0 dB). Levels 1 and 2 each have an on/off row AND a level row; level 3 had only the on/off row. |
| R5-5 | **The link mode moves onto the channel.** An output carries `outputArray` (membership) AND `outputApplyToArray` (0 OFF / 1 ABSOLUTE / 2 RELATIVE, per output). Effects have membership but only the global `effectsGlobalLinkMode`, so detaching one channel detaches every group. Add `effectLinkMode` on `<Channel>`; demote the global to the default a new channel is stamped with. |
| R5-6 | **Propagation must consult the RECEIVER's mode.** `WFSValueTreeState.cpp:1186` reads each member's own mode and skips members set to OFF. The plan says clone `ClusterParamEdit.h`, and clusters have membership with **no per-member mode** — cloning it inherits exactly the gap R5-5 closes. Model on `ArrayParamEdit.h` + the array propagation at `WFSValueTreeState.cpp:1157-1250`. |

**The three matrix levels**, as the operator framed them:

| Level | Matrix | Mute | Level |
|---|---|---|---|
| 1 | inputs → an effect's entry | `effectSendOns`, 64 wide, keyed by input PERMANENT number | `effectSendLevels`, -92..0 dB |
| 2 | effect outputs → other effects' entries | `effectFxSendOns`, 32 wide, dense effect index, diagonal off | `effectFxSendLevels`, -92..0 dB |
| 3 | effect outputs → outputs | `effectMutes`, one token per LIVE output | `effectArrayAtten1..10` (R5-4, not yet added) |

**Level 3 must NOT become a free per-output matrix.** Levels 1 and 2 are true mixers. Level 3 is
not: an effect return is a WFS render source, so its per-output gains are SOLVED from the geometry
of the return position against each speaker. An arbitrary per-output level would overwrite the
spatialisation that makes the return localise where its marker sits. The family offers exactly two
overrides on top of the solution — a per-output MUTE and a per-ARRAY TRIM — and that is why level
3's surface is deliberately smaller.

---

## 6. Verified mechanically — do not redo

- **All 104 spatcore `EffectParams` defaults agree exactly with the app's `effect*Default`
  constants.** The one unpaired field, `inputTrimLin`, is marked "reserved, not exposed in v1".
- **491 numeric cells in `WFS-UI_effects.csv` equal their constants.** Zero drift, checked by two
  independently written verifiers.
- **No prefix dispatch on an effect name exists outside `getParameterScope`,** where the two tests
  are correctly ordered `effectsGlobal` before `effect`. The prefix-collision hazard the identifier
  header warns about (`effectDist`/`effectDistance*`, `effectDelay`/`effectDelayLatency`,
  `effectSend`/`effectSendLevels`, `effectMute`/`effectMutes`) is entirely FUTURE — it bites in the
  OSC parser, OSCQuery and codegen. Order those tests longest-first or compare with `==`.
- **Both app↔spatcore mirror constants are now static_asserted** (`WFSCalculationEngine.cpp`), so
  `numEffectModuleSlots` and `maxEffectChannels` cannot drift silently.

---

## 7. How this branch works — the part that is not optional

**Every serious bug in this phase was found by review, not by writing, and every one was the same
mistake:** code inferring what it may destroy from what it cannot see.

1. The eviction hook treated an empty template node as proof every property on it was retired —
   it would have deleted the whole send matrix on every project open.
2. The load path had no template to merge onto, so a half-built channel loaded as a live one.
3. The output-count refit truncated a mute row on a shrink, destroying the very row it was added
   to protect.
4. The row guard checked the value's TYPE, not its SHAPE, so a one-token string became a
   full-width row of defaults — reached through a shipped tool.

**Mutation-test every gate you add.** Break the mechanism on purpose, rebuild, confirm the
assertion fails, restore. **Three gates on this branch could not fail when first written** and
were caught only this way. The most recent: a scalar lands on column 0, and every row was idle at
column 0, so the assertion passed for the wrong reason.

**Gate commands** (from `D:\dev\WFS_DIY_v1`):

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" `
    Builds\VisualStudio2022\WFS-DIY.sln -p:Configuration=Release -p:Platform=x64 -m
# structural self-test: WFS_TEST_CHANNEL_LIST=1, launch the Release exe, read the log.
# BACK UP %APPDATA%\WFS-DIY\WFS-DIY.settings first and restore it after — the forced kill
# flips cleanShutdown.
```
```bash
cd tools/validation/control-replay
python session_roundtrip.py      # six section files, effects.xml included
python osc_replay.py
python oscquery_echo_check.py
python mcp_replay.py
python midi_snapshot_check.py    # needs the teVirtualMIDI port
python fade_ramp_check.py
python remote_tablet_mock.py
cd ../offline-render && ./build/.../offline-render.exe --path cpu --check baselines/win-dev-nvidia.json
```

A moving golden is a finding, not something to `--update` away. The two exceptions so far were
both deliberate and stated in their commit messages.

**The app is single-instance**, so only one agent may drive it at a time, and two MSBuild runs on
the same obj directory corrupt each other. Parallel agents must be scoped to disjoint files and
only one may build.

---

## 8. Known-open, none blocking

**From Phase 5:**

- **`effectArrayAtten1..10` is a zero-filled hook** (R5-4). The return rows apply a per-array trim
  keyed by `outputArrayAssignments`, and every array trims by 0 dB until the identifiers land with
  the control surface. Phase 6/7.
- **Binaural monitoring mutes every effect return whenever any input is soloed**, which is the
  reverb-tap rule and the user's decision of 2026-09-17. It holds by construction rather than by a
  branch of its own: binaural gates a source on its OWNING CHANNEL's solo bit, a return owns no
  input channel, and the bitmask reports false for -1. Binaural renders returns at their positions
  through the same kind-aware accessor the WFS path uses, so the origin hazard is closed on both
  paths. The Effects tab's Solo Effects sets the mask since Phase 6.
- **The reverb `handleConfigReloaded` gap is unchanged.** A project load whose reverb count differs
  from the prepared one does not call `setNumNodes`, resize the buffers or re-prepare the return
  processor. The effects path has the guard the reverb path lacks (it stops processing when the
  prepared layout no longer matches); fixing the reverb twin was explicitly left out of scope.
  **Only a project load can fire that guard.** The other route into `handleConfigReloaded` is
  snapshot recall, which is reachable remotely over OSC, but every entry of
  `ExtendedSnapshotScope::getScopeItems()` targets a per-channel subsection — Channel, Position,
  Attenuation, AutomOtion, Directivity, GradientMaps, Hackoustics, LFO, LiveSourceTamer, Mutes,
  Sampler, ADMMapping — and not one targets `Config` or `IO`, so a snapshot cannot carry a channel
  count of any family. A cue therefore never changes the render-source layout.
- **`effectsGlobal*` other than the loop-guard switch apply at the next Processing start**, because
  the rest of them are config the engine reads in `prepare`.
- **The binaural-only path never drives the effects engine.** It pops nothing and notifies nothing,
  so returns are silent there.
- **The engine's meters are peaks only.** `getEffectLevel` reports the same number as its RMS field
  because the engine taps no mean square. The AutomOtion trigger on an effect therefore reads the
  return row's render-source meter, which has a true one, rather than the engine tap.
- **The engine's per-channel peaks have no ballistics of their own** and are overwritten every
  batch, which is why the app polls them at 5 ms. Decayed peak atomics in spatcore would let the
  poll go back to the metering tick. Follow-up, spatcore side.
- **`maxFeedDelaySeconds` is a flat 1.0 s.** At 136 sources and 96 kHz that is about 52 MB of feed
  history allocated in `prepare`, only when effect channels exist. A geometry-derived cap (the
  longest published feed delay plus a block) is the follow-up.
- **spatcore's `RenderSourceMap.h:113-119` comment and the `kMaxRenderSources` alias are stale** —
  the alias still reads 104 while `kMaxRenderSourceSlots` is 136. Doc-only spatcore PR.
- **The control-replay fixture stays at `effectChannels="0"`.** Phase 8.

**From earlier phases:**

- **The reverb EQ band accessor is unguarded and HAS live callers** — an unknown child in a reverb
  `<EQ>` makes a band write land on it and report success. The effects twin was fixed; the reverb
  one was left because it is shipped behaviour outside this phase.
- **The reverb mute tool takes a string enum with no output argument**, so it cannot address a
  column at all. It is now a logged no-op rather than a row-destroyer, but still reports success to
  the client. Fix with the C9/C10 grid tools.
- **The OSC per-output mute form exists for `inputMutes` only.**
- **The output EQ dispatcher ignores its band index entirely**, so a generic `eqGain` write always
  lands on band 1. Shipped.
- **`effectsMapVisible` is read by the Map and the Effects tab header since Phase 6** (it used to
  be stamped, persisted and routed with no reader).
- ~~**Snapshot/MIDI/QLab scope has zero effects coverage.**~~ Closed by Phase 7 (§10): one snapshot
  carries both families, and the scope has an effects grid.
- **MCP session-info reports per-family channel counts without effects**, so a session with 8
  effect channels is described as having none.
- **`effectMuteMacroMax = 4`** reaches only mute-all, unmute-all and invert; the reverb equivalent
  implements two more (mute-odd, mute-even). The plan specifies 4, so the constant is faithful —
  but the plan looks like it truncated the list by oversight. **Product decision, one constant.**
- **A deliberate trade-off, not a bug:** "never cut a per-output row" means a mute on an output the
  rig does not currently have survives UNMUTE ALL and the other grid macros, because the grid
  speaks only for the outputs it shows. The mute reappears with the outputs.

---

## 9. Phase 6 — the Effects tab: DONE

Fourteen commits on `effects/phase-6` (2026-09-22), each gated on a Release build, the
`WFS_TEST_CHANNEL_LIST` self-test, the seven control replays with no golden regenerated, the
offline-render CPU canary, the kernel hashes, the dependency lint and the bounds audit. Every new
assertion was mutation-tested (phases A, L, O7, G1/G2 and the OSC replay's verb needles). One
spatcore commit (PR #14, tagged v0.3.3) added the schema-free `ui/sends/SendMatrixComponent` and
`EffectsEngineCore::getSlotMeterDb`.

| Commit | What it did |
|---|---|
| schema (G0) | `effectLinkMode` on the channel (the output-array model) and `effectArrayAtten1..10` on the return, read by the calc engine; the CSV registered in the bounds audit's own list (R5-7). |
| link funnel (G1) | `EffectParamEdit.h`, the effects twin of `ArrayParamEdit`: propagation reads the RECEIVER's mode, mutes never propagate (R5-1), a group mute is an ACTION on every member's `effectMute` (R5-2). Self-test phase L. |
| tab shell (G2) | `Source/gui/TabIndex.h` names every main-tab index; the Effects tab sits between Reverb and Inputs (Q14); the System Config count editor with its reduction dialog. |
| Channel Parameters (G3) | `EffectsChannelPanel.h` in the Reverb tab's three-column geometry, the link row at the top; the Map draws the returns (teal rounded square, otomo dot). |
| sends (G5) | `EffectsSendsPanel.h` + `EffectsSendMatrixShim.cpp`: the WHOLE matrix (inputs then effects as rows, effects as columns), the diagonal refused, entry and cycle badges; the header reworded to the Reverb tab's. |
| LFO (user request) | Fifteen `effectLFO*` on a new `<LFO>` node (the input set minus gyrophone), `LFOFamily` on `LFOProcessor`, a second engine offset slot that ADDS to the AutomOtion's, self-test O7; the Movements sub-tab in the Inputs tab's geometry. |
| Chain (G6) | `EffectsChainPanel.h` (link badge, chain bypass, latency, draggable tile strip with engine meters) + `EffectsModulePanel.h` driven by `EffectsModuleDescriptors.h`, GENERATED from the CSV by `tools/gen_effects_module_ui.py` with the strings; EQ display, GR meter, tap rows and reverb presets special-cased. Self-test G1/G2. |
| Settings (G7) | `EffectsSettingsPanel.h`: the nine globals through `setConfigParam`, Re-layout clears the effects latch; an `<EffectsGlobal>` write refreshes every reader of the group names. |
| Stream Deck (G9) | `EffectsTabPages.h`: five pages, every per-channel write through the funnel, the Chain page driven by the same descriptors. |
| OSC verbs (G10) | selected / editOnMap / clear / clearAll received; each accepted verb and every refusal writes a session-log line the OSC replay asserts. Snapshot verbs still refused, naming phase 7. |
| meters (G11) | The level-meter window shows a feed / return pair per effect and the engine's duty. |
| help + docs (G12) | Five help cards, this section, the plan's §12.8, CLAUDE.md, the change log. |

**What the interface forced back** is the plan's §12.8. The ones a reader of THIS document needs:

- The reverb module applies NO preset from `effectReverbType`; the GUI writes the preset's eight
  values through the funnel on selection and turns the type back to Custom on an edit. The CSV's
  default type (Room, 0) does not match its default values - still open.
- `getSlotMeterDb (fx, slot)` indexes the DECLARED slot (`kSlots` order), not the chain position;
  Dynamics report gain reduction, everything else its output peak; -120 dB is the "no engine"
  sentinel and the GUI treats anything below -60 dB as silence / no reduction.
- Nine of the eleven modules need no code per control: the CSV row is the contract, the generator
  emits the descriptor and the strings, and the GUI panel and the Stream Deck page both read them.
- The Effects tab was not screenshot-verified for the Settings sub-tab (the dev box locked) and the
  Stream Deck pages were not exercised on hardware (none attached); everything else was captured.

---

## 10. Phase 7 — snapshots carry the effects: DONE

Ten commits on `effects/phase-7` (2026-09-22/23), off the Phase 6 tip. The design changed before a
line was written (the user, 2026-09-22): the effects REUSE the input snapshots - one file, one scope
with a grid per family, one Scope window with a tab per family, reachable from both tabs - instead
of the plan's second snapshot family. The plan's §12.9 (revision 8) records it and the body was
amended to match. The three questions it raised were answered: the `/wfs/effect/snapshot/*` verbs
are RETIRED (refused with a pointer), the Effects tab carries the FULL snapshot row, and Write to
QLab exports PER-PARAMETER effect cues.

| Commit | What it did |
|---|---|
| `48652ff` scope matrix | `ScopeMatrix` over a `ScopeItemTable`; `ExtendedSnapshotScope` keeps its API as forwarders. Snapshot files byte-identical to the pre-phase exe (OnSave, OnRecall, full, diffed). |
| `3b4c681` effects grid | `effectScopeTable` (property items on the flat nodes, one whole-node item per module), `<EffectsScope>` in `<ExtendedScope>`, `EffectsSnapshotScope.h`, self-test Q. |
| `e339a86` store / recall | `<Effects>` in the file; apply writes only what the live node has, rows through `setEffectParameter`; skipped ids reported; ghosts kept; each half in its own undo domain; OnSave trim. Self-test N0-N10. |
| `7792d57` dirty tracker | Effect writes mark their item (`itemIdFor`, a band reports its module). N11. |
| `5d8beae` shared row | The Inputs tab's snapshot code moved into `SnapshotSession` + `SnapshotRow` (`Source/gui/snapshots/`), owned by MainComponent before the tab container. Behaviour unchanged. |
| `af4b75a` two-tab window | Inputs / Effects tabs in the Scope window; `editScope (family)`. |
| `e766009` Effects footer | The shared row on the Effects tab (second footer row) + `WFS_TEST_RENDER_UI`. |
| `4952972` QLab | Per-parameter effect cues in each parser shape, `getEffectMappings`. N12 / N13. |
| `6a73bc2` retired verbs | The refusal names `/wfs/input/snapshot/*`; the OSC replay asserts it; CSV rows; MCP description. |
| docs | Plan §12.9 and amendments, this section, CLAUDE.md, help card, hover strings, change log. |

**Gates on every code commit:** Release build; `WFS_TEST_CHANNEL_LIST=1` ALL PASS (750 -> 811);
the seven control replays PASS with no golden moved; every new assertion mutation-tested (17
mutants across Q, N, N11, N12, N13 and the OSC replay's new needle, every one caught). At the end:
offline-render CPU check zero MISMATCH (the five MISSING stereo combos are pre-existing), kernel
hashes, dependency lint, bounds-audit counts identical to the pre-phase commit, `pytest tools/mcp`
25 passed.

**What a reader of THIS document needs:**

- **The effects half is node-driven for the modules.** Anything new that walks a snapshot's
  effects - an importer, an MCP tool, a remote - must resolve items through
  `EffectsSnapshotScope::itemIdFor (childOfEffect, property)`; `hasProperty` alone cannot tell
  FxEq1 from FxEq2, or band 3 from band 4.
- **Recall never goes through the link funnel** and writes rows through `setEffectParameter`: the
  fx diagonal a hand-edited file offers comes back off (N6).
- **The QLab export is big**: about 275 cues per effect channel in scope. The snapshot-load cue (one
  cue recalling the whole file) is the lighter path and covers the effects.
- **`WFS_TEST_RENDER_UI=<folder>`** renders the tabs and the Scope window offscreen - use it when
  the dev box is locked; `createComponentSnapshot` does not need the screen.
- **Known, not fixed:** `WFS_TEST_MUTES_PERSIST=1` fails the same 8 checks (M2-M5, mute-list width
  and one-output mutes) on the pre-phase exe as after it - pre-existing, outside this phase. The
  full-tier locales still say "Input Snapshot" in the row's hover text (en was reworded).
- **Not verified on screen:** the long-press actions from the Effects tab's row (the workstation
  locked mid-session); the rows, the window's tabs and Edit Scope were verified by offscreen renders
  and, before the lock, a live capture of the Inputs tab's row and window.
