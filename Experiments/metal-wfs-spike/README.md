# Native Metal WFS kernel spike

Headless prototype proving the WFS delay-and-sum routing kernel on **raw Metal**
(no GPU Audio SDK): correctness against a CPU reference + dispatch-latency and
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
matrix    | sync dispatch ms (mean/p99/max)  | pipelined ms (mean/p99) | correctness
  8x16    |  0.135 /  0.155 /  0.234        |  0.022 /  0.059         | maxDiff 8.9e-08 PASS
 16x32    |  0.134 /  0.202 /  0.246        |  0.010 /  0.069         | maxDiff 1.2e-07 PASS
 32x64    |  0.127 /  0.181 /  0.229        |  0.007 /  0.055         | maxDiff 2.4e-07 PASS
 64x64    |  0.162 /  0.279 /  0.329        |  0.008 /  0.021         | maxDiff 2.4e-07 PASS
 64x128   |  0.172 /  0.288 /  0.568        |  0.009 /  0.026         | maxDiff 2.4e-07 PASS
```

## Comparison vs the GPU Audio SDK path (same machine, same day, gpu-audio-v2)

| Metric | GPU Audio SDK | Raw Metal (this spike) |
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
   gpu-audio-v2 — it is SDK-agnostic except the inner Execute/Retrieve calls).
2. Triple-buffered params/matrix upload (rotating buffers; this spike reuses one
   set, fine for benchmarking, not for overlapped launches with changing matrices).
3. Partitioned-convolution kernel for IR reverb (FFT: MPSGraph or hand-rolled
   radix shared-memory FFT; structure understood from the SDK adaptation work).
4. CUDA twins of both kernels for the Windows/NVIDIA rackmounts (cuFFT).
