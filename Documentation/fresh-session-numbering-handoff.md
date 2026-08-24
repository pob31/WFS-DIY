# Handoff — fresh-session channel numbering, per-type names, stereo glyph, slot/number sweep, input patch re-flow

> **2026-08-24 addendum — the latch-trigger set described below changed.** Display-only acts
> (Inputs/Map tab selection, Level Meter open, patch-window open) no longer latch; the first
> **project save** (`saveSystemConfig`/`saveInputConfig`) and the first **actual patch edit**
> (`PatchMatrixComponent::onBeforeUserPatchEdit`) now do, and every latch logs
> `"Channel numbers latched: <reason>"` on the fresh→owned transition. The current trigger list
> lives in `Documentation/CLAUDE.md` (*Fresh-session compaction* → *Latch triggers*); this file is
> otherwise historical.

Written 2026-08-20. Branch `feat/stereo-input-channel` in **both** repos (app + `spatcore`).
App HEAD `793b614`, spatcore HEAD `63e24a7`. **Everything below is UNCOMMITTED working-tree state.**

## State at handoff

| | |
|---|---|
| App files changed | 43 (uncommitted) |
| spatcore files changed | 4 (uncommitted) |
| Debug build | **green — 0 errors, 0 warnings** (`Builds/VisualStudio2022`, MSBuild `-p:Configuration=Debug -m`) |
| Channel-list self-test | **ALL PASS** (launch the exe with env `WFS_TEST_CHANNEL_LIST=1`, no project loaded; results land in `%APPDATA%\WFS-DIY\logs\WFS-DIY_*.log`, grep `SELF-TEST`) |
| Committed | **nothing** — not staged, not stashed |

Baseline was verified green *before* any edit, so any regression is attributable to this work.

## What landed

### 1. Fresh-session channel numbering (`channelNumbersUserOwned`)

A persisted one-way latch on `Config > IO`, modelled on the existing `positionsUserOwned`. While a session
is fresh, `compactChannelNumbersToDisplayOrder()` renumbers every input to `slot + 1` at the tail of
`addInputChannel` / `removeInputChannel` / `moveInputChannel`. Once latched, today's append-only
permanent-number regime applies unchanged.

Latches on: any project/config load, the input patch window, the Inputs (tab 4) or Map (tab 6) tab, the
Level Meter window, snapshot store/recall, inbound OSC/ADM/Remote naming an input by number, a Remote
handshake, QLab cue sends, and MCP tools naming an input (including create/delete).

**Invariants that must not be broken — each is load-bearing and non-obvious:**

- `areChannelNumbersUserOwned()` returns **TRUE when the IO tree is invalid** — deliberately the opposite
  fallback to `arePositionsUserOwned()`. "Unowned" licenses rewriting every channel id, so malformed state
  must land on the permanent regime.
- The property is **never stamped into `createIOSection`** or any schema template. `ensureCompleteSchema`
  → `backfillFromTemplate` copies every template property the loaded tree lacks, so a templated `false`
  would mark legacy sessions "fresh" and renumber a real show. *Absent = fresh* plus *every load latches*
  is the whole backward-compatibility story.
- **Outgoing OSC does not latch.** It fires continuously; latching there would close the window
  milliseconds after startup and the feature would silently do nothing.
- In the compaction the **`id` write must precede the `inputName` write**: `TreeParameterStore` is a
  `ValueTree::Listener`, so raw `setProperty` dispatches synchronously and `resolveChannelIndex` resolves
  an Input parent's slot via `getSlotForChannelNumber(parent.id)`, not `id - 1`.
- The compaction uses **raw `setProperty(..., nullptr)`**, never `setParameter`/`setInputParam`: those
  carry undo and dirty-tracking side effects *and* clamp against `OSCParameterBounds`, where
  `inputTrackingIDMax` is 32 while channels go to 64.

### 2. Per-type default names

Default names are now `Mono N` / `Stereo N` on **independent per-type counters in display order**, not
derived from the channel number. `resequenceDefaultInputNames()` runs when the **Arrange window closes**
and only while the session is fresh (hooked via `ModalComponentManager::attachCallback` in
`SystemConfigTab::openChannelListEditor`, the idiom already used at `NetworkTab.h:4361`). A move alone
renames nothing; a user-renamed channel always keeps its name. `compactChannelNumbersToDisplayOrder()` no
longer writes `inputName` at all.

New channels take the **highest ordinal any live name of that shape already claims, plus one** — never the
count of channels of that type. A latched session never resequences, so after a delete `count + 1` would
permanently hand out a name a live channel still carries.

`Mono`/`Stereo` are **deliberately un-localised**, matching the pre-existing hard-coded `Input N`: these
strings persist into the tree and into project files, so localising them would mutate stored names on a
language switch and break the "is this still a default name?" test across languages.

### 3. Stereo glyph

The classical two-circle mark: circles of equal radius with centres **1.5 radii apart**, outlined, never
filled; aspect-preserving fit, stroke `jmax(1.0f, fittedHeight * 0.10f)`.

- **Picker tiles** — stamped in `ChannelSelectorOverlay::paintOverChildren`, immediately right of the
  centred top-line number, which stays exactly where JUCE centres it. Opt-in via
  `setChannelStereoProvider`, wired only in `InputsTab`, so Outputs and Reverb pickers are untouched.
- **Input patch rows** — `PatchMatrixConfig::showRowCapacityBadge` (default `false`), set only for the
  input patch in `PatchMatrixShim.cpp`. The row-header strip is **widened** to absorb the gutter, so
  channel names lost no space; the gutter is reserved on every row so the id column keeps a straight left
  edge.

### 4. Slot-vs-number sweep (27 sites)

An exhaustive sweep found and fixed 27 places conflating an input **slot** with its permanent **number** —
far more than the 8 originally spotted. Fixed across `MainComponent.cpp` (14), `OSCManager.cpp` (4), the
MCP tools (`SetParameterTool`, `SetParameterBatchTool`, `NudgeParameterTool`, `SessionTools`,
`StateDeltaTool`, `MCPUndoEngine`), `QLabCueBuilder.h` (2), `SamplerSubTab.h`, and
`spatcore/control/mcp/MCPChangeRecords.h`.

These were **pre-existing bugs**, not regressions — latent only while numbers happened to equal `slot + 1`,
which the reorder/delete work makes routinely false. They address or report the wrong channel silently,
with no error.

**Scope rule, critical:** output, reverb and cluster ids *are* dense slot positions, so `index = id - 1`
and `id = index + 1` are correct for them and were left alone. Every fix is scope-gated.

### 5. Stereo width redefined in metres, with an orientable spread axis

`inputStereoWidth` was a percentage of **half the speaker array's X extent** — a reference that is wrong on
every non-frontal rig: a circular array always spread ±radius, side arrays were scaled by an axis they barely
extend along, and on a straight array running along Y (`halfSpanX == 0`) the dial did **nothing at any
setting**. It is now a **float, 0–50 m, default 4 m**, the full L↔R distance, with the pair's position as the
centre. The mapping reads **no speaker positions at all** — that is what makes it geometry-independent.

New `inputStereoAxisOffset` (int, −179..180°, default 0) is a **rotation applied to** the automatic tangential
axis, not an absolute angle plus a mode flag — one control, 0 = automatic, ±180 = an explicit L/R swap, and a
nudge stays applied as the source moves. Near the origin the automatic axis is now **frozen below a 1 m
radius** (latched, continuous at the boundary by construction) instead of whipping 180° and snapping to +X,
so a pair at the centre of an in-the-round rig is stable and explicitly orientable.

Supporting changes: a new header-only `Source/Helpers/StereoImageGeometry.h` holds the single implementation
(unlisted in the `.jucer`, like `CoordinateConverter.h` — no Projucer resave); `MapTab` now **reads the
resolved legs back** from the engine through `setStereoImageCallback` instead of re-deriving the formula, so
the two can no longer drift; `StereoDecomposerConfig::widthFactor` is gone from spatcore. `inputStereoWidth`
had never shipped (only on this branch), so the unit changed in place with **no migration path** — do not add
one. New self-test `WFS_TEST_STEREO_GEOMETRY=1` (18 assertions) proves the headline invariant: a 4 m pair
measures 4 m at **all 360 bearings** on a circle, worst error 0.5 µm.

### 6. Fresh-session input patch re-flow

While a session is fresh the **input patch** re-flows with the channel list.
`compactInputPatchToDisplayOrder()` walks the rows top to bottom handing out **consecutive** hardware
input columns — two adjacent columns for a stereo row (lower = L), one for a mono row — leaving a
gapless diagonal in display order. It runs in the same `if (! areChannelNumbersUserOwned())` block as
`compactChannelNumbersToDisplayOrder()` at the tail of `addInputChannel` / `removeInputChannel` /
`moveInputChannel`, **plus** at `setInputChannelType`, and shares that one latch: there is deliberately
**no second, patch-specific ownership flag**, since a patch is fresh precisely when the numbers are.
The `setInputChannelType` asymmetry is deliberate — a type flip moves no channel, so compacting numbers
there is a provable no-op, but it changes that row's capacity by a column and shifts every column after it.

The repro that motivated it: 8 default monos, add two stereo pairs, add four monos, drag one of the new
monos between the pairs. Before, Mono 9 kept hardware input 15 and Stereo 2 kept 11+12, leaving holes
nothing could close. Now 1-8 → 1..8, Stereo 1 → 9+10, Mono 9 → 11, Stereo 2 → 12+13, Mono 10 → 14,
Mono 11 → 15, Mono 12 → 16. Packing is strict and deliberately **not** aligned to the interface's
odd/even pairs — a pair may start on input 11 — which is what makes N mono + M stereo always fit in
N + 2M inputs. The diagonal may also run past `activeHardwareInputs`: `applyColsPolicy` widens `cols`
and the matrix dims those columns, and clamping would make the patch depend on which interface happened
to be plugged in at edit time.

The compaction **rebuilds from the channel list and discards the stored rows**. That is safe only
because of a call-graph property: while unlatched nothing outside the channel list has ever written
`patchData`. The sole interactive writer is `PatchMatrixComponent::savePatchesToValueTree`, reachable
only through `MainComponent::openAudioInterfaceWindow()`, which latches *before* constructing the
window; every load path latches on success; MCP lists `patchData` under `ignored_parameters`; OSC has
no patch address. **Anything future that authors a patch must latch first or the re-flow will eat it.**
Rebuilding is idempotent, self-repairs a stale row count and the ragged rows `insertInputPatchRow`
leaves, and is a **fixed point of the reconfiguration tail** — `normalizeInputPatchRows`,
`sanitizeMonoPatchRows` and `autoPatchStereoRightColumns` all no-op on it — so the tail can neither
perturb the diagonal nor add a write.

## Review status — closed

The slot/number sweep was reviewed adversarially (10 findings raised, 8 confirmed after independent
refutation) and **all 8 confirmed findings are applied and verified**:

- the QLab export now actually passes the `numberToSlot` resolver at both call sites, so the resolver is no
  longer inert (it was: entries for a number past `numChannels` were silently dropped, and a reordered list
  filtered cues through the wrong channel's scope column);
- `session_get_state` latches, now that it publishes real permanent numbers rather than `slot + 1`;
- batch undo carries the permanent number (`ChangeSubWrite::channelNumber`, 0 = dense slot) and re-resolves
  it at undo time instead of replaying a stale slot — the MCP undo ring survives `clearAllUndoHistories()`,
  which only clears the JUCE UndoManagers;
- the Clusters tab, Stream Deck map pages, AutomOtion status strings and the tracking-conflict dialog all
  display the permanent number while their routing keeps using slots.

An independent verify pass confirmed no over-application (output/reverb/cluster keep `id - 1` / `index + 1`)
and that every substituted call is bit-identical in the ordinary dense case, since
`getSlotForChannelNumber` has an explicit dense fast path.

## Next session — do these first

1. **Decide the `[L3 R4]` question.** On stereo input patch rows the textual leg names were **removed** in
   favour of the glyph — the suffix cost ~40% of the label strip, enough to clip a name as ordinary as
   "Grand Piano". The user did not ask to lose that text. Either confirm, or widen the header further so
   both fit. Guarded by `badgeWidth == 0`, so hosts without the flag are unchanged.

2. **Commit order matters.** `spatcore` must be committed first, then the app's submodule pointer bumped in
   the same app commit as the code that depends on it.

3. **Two nuances flagged by the verify pass, neither blocking.** `ClustersTab.h` now suppresses the name
   suffix for any app-stamped default shape, so a channel a user deliberately typed as "Mono 3" is treated
   as a default and its name is not appended to the row — cosmetic, and the same rule
   `resequenceDefaultInputNames` already uses. And `InputsTab.h:8273-8281` captures the slot into the modal
   callback and resolves the number after it returns; the pre-existing write on that path does the same and
   it is unreachable in practice (the modal blocks stopped-only structural edits), so the fix inherited the
   pattern rather than closing it.

## Deliberate non-actions (do not "fix" these)

- `Resources/lang/full/` untouched — a stale 2026-08-02 Full-tier overlay that lacks every affected key, so
  it falls through to the corrected `en.json`. Touching it is what would introduce a falsehood.
- `Documentation/proofreading/` checklists not regenerated — `tools/gen_proofreading_checklists.py` would
  emit ~10 unrelated stereo-rework entries per locale and swamp the diff. Separate pass.
- No `change_log.md` entry — the whole stereo rework on this branch is absent from it, so it is written at
  release time, not per-commit.
- `SessionTools.h` / `StateDeltaTool.h` do not *latch* (they run on essentially every MCP session and would
  kill the feature). They were still fixed for the slot/number conflation, which is a separate concern.
- `insertInputPatchRow` / `moveInputPatchRow` are **not** simplified away now that the patch re-flows.
  While unlatched their result is deliberately overwritten a few lines later; once latched the re-flow
  never runs and they are the entire story. Deleting them would leave the permanent regime with no way
  to carry a patch row alongside its channel.

## Verification recipe

```
cd "d:\Dev\WFS_DIY_v1\Builds\VisualStudio2022"
"C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" WFS-DIY.sln -p:Configuration=Debug -m
```
Then launch `x64\Debug\App\WFS-DIY.exe` with `WFS_TEST_CHANNEL_LIST=1` **and no project auto-loading**
(`setProjectFolder` at startup does not load config, so a cold start stays unlatched); kill it after ~20 s
and grep the newest `%APPDATA%\WFS-DIY\logs\WFS-DIY_*.log` for `SELF-TEST`.

**Check for `SELF-TEST SKIP U` first.** The U phase bails out when the numbers are already user-owned —
anything that latched before the test ran (an auto-loaded project, a tab selection) skips it silently, and
`ALL PASS` with a skipped U phase proves nothing about the fresh-session behaviour the phase exists to
cover.

With the phase actually running, expect `RESULT: ALL PASS` including U0a–U6: fresh-session renumbering,
per-type names, custom-name survival, latched no-op, and **U0a / U0b** — the eight-mono / two-pair /
four-mono / drag-a-mono-between-the-pairs repro, asserting both the 1..14 numbering and the strictly
packed patch diagonal with two adjacent columns per stereo row.

Language files: all nine must parse and **no key may be added or removed** — values only. The non-English
files are partial translations that fall back to `en.json`, so cross-file key parity is *not* the invariant;
"key set unchanged vs HEAD, per file" is.
