# Native Metal WFS kernel spike

Headless prototype proving the WFS delay-and-sum routing kernel on **raw Metal**
(no vendor SDK): correctness against a CPU reference + dispatch-latency and
scaling benchmarks. First step of the native GPU backend (`gpu-native` branch).

- `wfs_delay_sum.metal` — MSL port of the delay-and-sum kernel (gather block per
  output, append block per input, persistent per-input rings with host-tracked
  positions, per-sample prev→curr interpolation of delays and gains).
- `spike.mm` — command-line harness: streams 50 blocks against a CPU reference
  (rings + ramps + history exercised), then measures 500 sync dispatches
  (commit + waitUntilCompleted) and 500 pipelined ones (depth 4).
- `./build.sh && ./spike` (no Xcode project; kernel source compiled at runtime).

## Results — Mac mini M4 Pro, macOS 15.7, block 128 @ 48 kHz (budget 2.67 ms)

```
matrix    | sync (mean/p99/max)      | parked-event (mean/p99/max) | pipelined (mean/p99) | correctness
  8x16    | 0.150 / 0.168 / 0.280   | 0.134 / 0.177 / 0.233      | 0.022 / 0.084        | PASS (9e-08)
 16x32    | 0.141 / 0.179 / 0.267   | 0.117 / 0.169 / 0.289      | 0.012 / 0.071        | PASS
 32x64    | 0.137 / 0.212 / 0.377   | 0.122 / 0.136 / 0.227      | 0.007 / 0.044        | PASS
 64x64    | 0.161 / 0.277 / 0.332   | 0.141 / 0.219 / 0.425      | 0.009 / 0.026        | PASS
 64x128   | 0.166 / 0.285 / 0.574   | 0.146 / 0.220 / 1.739      | 0.013 / 0.044        | PASS
```

Three execution modes:
- **sync**: encode+commit+wait per block (worst case, comparable to the SDK's
  in-callback Execute).
- **parked-event**: command buffers pre-committed and gated on an
  `MTLSharedEvent`; per block the CPU only writes input and signals — the
  supported variant of a "persistent kernel" (no watchdog risk, no spinning).
  On a quiet desktop it saves only the encode/commit (~20 us); the open
  question is whether pre-scheduled work survives compositor bursts better —
  measure by running ./spike during active desktop use.
- **pipelined**: commit without waiting, depth 4 (the GpuAsyncPipeline model).

## Comparison vs the earlier vendor-SDK prototype (same machine, same day)

| Metric | Vendor SDK | Raw Metal (this spike) |
|---|---|---|
| Sync dispatch floor, quiet desktop | 0.8–1.2 ms | **0.13–0.17 ms** (~7×) |
| Pipelined submit round-trip | 0.02–0.2 ms | **0.007–0.022 ms** |
| 64×128 (8192 routing pairs) | — | 0.17 ms mean = 6 % of budget |

Notes:
- The compositor-induced dispatch tail (3–5 ms under window churn / video starts,
  measured extensively on the SDK path) is OS/hardware physics and applies to raw
  Metal too — the deep-pipeline architecture (GpuAsyncPipeline) remains necessary
  and carries over unchanged. What improves is the *floor* (≈7× lower), i.e. the
  headroom everywhere.
- Unified memory: `MTLResourceStorageModeShared` buffers — zero transfer machinery.
- Licensing: 100 % ours. No SDK, no engine binary, GPL-compatible, CI-shippable.

## Next steps on this branch
1. `MetalComputeBackend` for GpuAsyncPipeline (port the pipeline class from
   the SDK prototype — it is SDK-agnostic except the inner Execute/Retrieve calls).
2. Triple-buffered params/matrix upload (rotating buffers; this spike reuses one
   set, fine for benchmarking, not for overlapped launches with changing matrices).
3. Partitioned-convolution kernel for IR reverb (FFT: MPSGraph or hand-rolled
   radix shared-memory FFT; structure understood from the SDK adaptation work).
4. CUDA twins of both kernels for the Windows/NVIDIA rackmounts (cuFFT).
