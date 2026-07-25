# Handoff: port the node-parallel SDN kernel to Metal

For the next session **on macOS**. Written 2026-07-25, after the CUDA reshape
shipped in v1.0.0beta39 and the HIP port landed compile-verified. This is the
last backend carrying the old shape.

## Why (results already banked on CUDA)

`sdn_process` runs the whole network as ONE threadgroup — one SM/core on any
GPU — with a per-sample barrier. At 32 nodes that measured **4.5–7 ms against a
5.33 ms budget** on NVIDIA and destabilised the direct WFS path. The
node-parallel twin (`grid = numNodes`, one warp per node, gather/scatter
parallel, arithmetic order untouched) measured:

| 32 nodes, 48 kHz / 256 blocks | lockstep | node-parallel | |
|---|---|---|---|
| RTX 5070 | 4.52 / 4.84 ms | 0.319 / 0.421 ms | 14.2× |
| Tesla T4 | 7.01 / 7.18 ms | 0.426 / 0.565 ms | 16.4× |

Bit-exact per device (15/15 GPU goldens unchanged on both). Expect the same
*shape* of win on Apple GPUs, not the same numbers.

## Reference implementations (read these first, port mechanically)

- **`spatcore/gpu/CudaSdnKernels.h`** — `sdn_process_nodes`, the kernel to
  mirror. The long comment above it explains the two-barrier structure and why
  no third barrier is needed. The header comment explains the chunk contract.
- **`spatcore/gpu/CudaSdnBackend.cpp`** — host side: second kernel handle,
  per-block mapping choice via `m.cfg.chunkSamplesForBlock (m.blockSize)`,
  `kMaxSdnChunksPerBlock = 8` fallback, chunked launch loop with
  `sampleOffset`/`chunkSamples`, prepare()-time warmup of BOTH kernels, env
  toggles. `HipSdnBackend.cpp` is the same pattern a second time.
- **`spatcore/gpu/SdnHostConfig.h`** — `chunkSamplesForBlock()` already exists
  and is backend-shared: **no host-math changes needed**.

## What to change

### 1. `spatcore/gpu/MetalSdnKernels.h` — add `sdn_process_nodes`

Mirror of the CUDA twin. MSL specifics:

- `SdnParams` gains `uint sampleOffset; uint chunkSamples;` at the END (keep
  field order identical to `SdnParamsGpu` in the backend — it goes through
  `setBytes`, so layout mismatch is silent garbage, exactly the bug the HIP
  port had).
- Node id = `threadgroup_position_in_grid.x` (grid = numNodes threadgroups);
  lane = `thread_position_in_threadgroup.x`, threadgroup size 32.
- `__shared__ float sIncoming[32]` → `threadgroup float sIncoming[32];` plus
  `threadgroup float sX, sDiffused;`
- Barrier A and B: `threadgroup_barrier (mem_flags::mem_threadgroup)` is
  sufficient for sIncoming/sX/sDiffused — but note the *scatter* writes go to
  device memory read by OTHER threadgroups only in later chunks (kernel-launch
  boundary orders them), so device-scope fencing between groups is NOT needed.
  Within the group, the gather in the next iteration reads only this node's
  incoming paths (device), written by other groups in previous *launches* —
  safe by the chunk contract.
- The per-sample loop bounds become
  `for (uint sc = 0; sc < p.chunkSamples; ++sc) { uint s = p.sampleOffset + sc; ... }`
  and `s` stays BLOCK-relative everywhere (`base = ringWritePos + s`, crossfade
  `mix + crossfadeRate * s`). Apply the same change to the existing lockstep
  kernel (CUDA did) so both kernels share the params struct.
- Keep the two sequential sums on lane 0 in path order — that is the
  bit-exactness contract (per device; CPU/GPU are NOT bit-identical, see
  `docs/core-boundary-proposal-audio.md` — the contract is float32-equivalence,
  and *unchanged Metal goldens* are the pass criterion).
- Fix the stale `N <= 16` comment at `MetalSdnKernels.h:16` while there
  (MAX_NODES is 32).

### 2. `spatcore/gpu/MetalSdnBackend.mm` — host side

- Second PSO: `newFunctionWithName: @"sdn_process_nodes"` next to the existing
  one (line ~140). Check `maxTotalThreadsPerThreadgroup >= 32`.
- `SdnParamsGpu` struct: add the two `uint32_t` fields, init
  `{0u, blockSize}` (see CUDA line ~285).
- In the process path (line ~386), replace the single
  `dispatchThreadgroups MTLSizeMake(1,1,1) / threadsPerThreadgroup N` with the
  CUDA backend's per-block logic verbatim:
  - `chunk = m.cfg.chunkSamplesForBlock (m.blockSize)`
  - fall back to the lockstep when `chunks > 8`, `N < 2`, or the env toggle
    says so
  - node-parallel dispatch: `dispatchThreadgroups MTLSizeMake(N,1,1)`,
    `threadsPerThreadgroup MTLSizeMake(32,1,1)`
  - per-chunk: update `sampleOffset`/`chunkSamples` in the params and
    `setBytes` again before each dispatch (params are per-dispatch on Metal —
    cheaper than CUDA's arg juggling; all chunks can go into ONE command
    buffer/encoder, which CUDA cannot do — do that, it keeps the per-block
    submission count at 1).
- Warmup at the end of `prepare()`: one sample through BOTH pipelines, then
  re-zero all state buffers (mirror `CudaSdnBackend.cpp` lines ~300-350 —
  the comment there explains why both kernels matter: pre-geometry sessions
  open on the lockstep and switch mid-session).
- `WFS_SDN_NODE_PARALLEL=0` escape hatch + `WFS_SDN_TRACE=1` mapping trace,
  read at prepare() (never getenv on the pump thread).

### 3. Validation (macOS)

```
python tools/validation/kernel_hashes.py --update        # intentional kernel edit
offline-render --path gpu-reverb-sdn --scenario all --check baselines/<mac>.json
```

Goldens must pass **UNCHANGED** — the reshape is transparent per device. If no
Mac GPU baseline exists yet, record one BEFORE the kernel change (that order is
the whole point). Then bench:

```
offline-render --path gpu-reverb-sdn --in 32 --sr 48000 --block 256 \
               --blocks 400 --bench          # lockstep vs WFS_SDN_NODE_PARALLEL
```

### Traps hit on CUDA/HIP — do not repeat

1. **Stale plugin/dylib**: if the harness loads a prebuilt GPU plugin rather
   than compiling the backend in, a kernel edit is invisible until the plugin
   is rebuilt — the goldens then pass *trivially*. An A/B (toggle on/off) that
   comes out identical to 4 decimals means the new code never ran. On macOS the
   backend is compiled into the app directly (no plugin DLL), so this likely
   does not apply — but verify with `WFS_SDN_TRACE=1` that the node-parallel
   mapping actually engages (`chunk > 1`).
2. **Struct layout**: the params struct exists twice (kernel string + host
   struct). Adding fields to one side is not a compile error anywhere — it is
   silent garbage. HIP shipped that bug for a few hours.
3. **Warm up BOTH kernels**, not just the one that runs first.
4. **`s` must stay block-relative** across chunks; `ringWritePos` advances once
   per block, not per chunk.

### Also pending on Metal (unrelated to SDN, same file family)

- CUDA-graph analogue: Metal already submits one command buffer per block, so
  there is nothing to port there (`change_log.md` beta39 notes this).
