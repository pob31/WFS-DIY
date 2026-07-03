# Phase 6 — Repo Split Runbook (joint session)

The mechanical procedure for carving `spatcore/` into its own repository and
mounting it back as a submodule. Prepared in advance; executed together.
Decisions marked ☐ are made at the session.

## Preconditions (verify before starting)

- [ ] All extraction branches merged; `main` green on every gate (app build,
      kernel hashes, dep lint, offline-render CPU+GPU baselines, 3 control
      replays, spatcore standalone tests)
- [ ] Namespace pass merged (spatcore fully `spatcore::*` with compat aliases)
- [ ] Working tree clean on all machines that will pull
- [ ] ☐ New GitHub repo created by owner (suggested: `pob31/spatcore`, empty,
      no README/license auto-files — history arrives via push)
- [ ] `git filter-repo` available (`pip install git-filter-repo`)

## Step 1 — Dry run the history carve (no risk, fresh clone)

```bash
git clone --no-local d:/dev/WFS_DIY_v1 /tmp/spatcore-carve && cd /tmp/spatcore-carve
git filter-repo --path spatcore/ --path-rename spatcore/:
```

Inspect: `git log --oneline | head -50`, `git log --follow rt/LockFreeRingBuffer.h`
— the `--path spatcore/` filter keeps only post-move history. ☐ Decide whether
pre-move history matters enough to add `--paths-from-file` with the old
`Source/...` paths of every moved file (richer blame, messier unrelated-commit
inclusion). Recommendation from the plan: try move-history-included in the dry
run; fall back to post-move-only if the result is noisy.

Also carve INTO the new repo: the six `docs/architecture/*.md` analysis docs +
`offline-render-harness.md`/`control-replay-harness.md`? ☐ Plan says the six
architecture docs migrate (they describe the core). Add `--path docs/architecture/`
to the filter if yes (they'd live at `docs/` in spatcore).

## Step 2 — Publish the new repo

```bash
cd /tmp/spatcore-carve
git remote add origin git@github.com:<owner>/spatcore.git   # ☐ URL
git push -u origin main
git tag v0.1.0 && git push origin v0.1.0
```

## Step 3 — Convert WFS-DIY to consume the submodule

On a branch (`spatcore/phase-6-submodule`):

```bash
git rm -r spatcore                      # removes the tree, keeps history
git commit -m "Phase 6: remove in-tree spatcore (moves to submodule)"
git submodule add <url> spatcore        # SAME PATH — includes stay valid
git -C spatcore checkout v0.1.0
git commit -m "Phase 6: mount spatcore as submodule at v0.1.0"
```

No `.jucer` or include changes needed — every include path already assumes
root `spatcore/`. The GPU build wrappers at `tools/{windows,linux}` already
point into `spatcore/tools/gpu/`.

## Step 4 — Full gate sweep on the converted checkout

Fresh `git clone --recursive` into a scratch dir, then: app MSBuild (v145) →
kernel hashes → dep lint → offline-render CPU+GPU `--check` → 3 control
replays → spatcore standalone tests → GPU plugin build via wrapper + smoke.
Everything must match the pre-split baselines bit-exactly.

## Step 5 — Follow-ups (same session or next)

- `tools/bump-spatcore.ps1` (fetch, checkout tag/SHA, run gates, commit bump)
- spatcore repo: its own JUCE pin (submodule) for `SPATCORE_STANDALONE_TESTS`,
  copy of the standalone wiring (today it reaches `../../ThirdParty/JUCE` in
  the WFS-DIY tree — must become self-contained), kernel-hash + dep-lint
  scripts relocated or duplicated into spatcore CI
- CI: spatcore standalone build + tests + consumer-matrix job (build WFS-DIY
  with the PR SHA overriding the submodule)
- ☐ juce_simpleweb: stays consumer-provided or relocates into
  `spatcore/third_party/` now (GPL accepted; relocation touches .jucer module
  path + Plugin/CMakeLists + the standalone wiring)

## Rollback

Pre-conversion `main` is untouched until Step 3's branch merges; the new repo
can be deleted/re-pushed freely until consumers pin it. Nothing in Steps 1-2
mutates WFS-DIY.
