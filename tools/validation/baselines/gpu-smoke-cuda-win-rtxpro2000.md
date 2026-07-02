# GPU smoke baseline - Windows x CUDA (dev machine)

Captured: 2026-07-02, branch spatcore/phase-0-harnesses (kernels per tools/validation/kernel_hashes.json)
Machine: Windows 11, NVIDIA RTX PRO 2000 Blackwell Laptop GPU
Driver: 596.72   Toolkit: CUDA 13.3 (NVRTC bundled)   Compile: arch-exact cubin (sm_XX)
Exit code: 0

```
loaded: .\wfs_cuda.dll   vendor: cuda   deviceIndex: 0
device: NVIDIA RTX PRO 2000 Blackwell Generation Laptop GPU (CUDA)
[A] WFS 1x1 g=1 d=0   out.mean=0.5000 (expect ~0.50)  peak=0.5000  launchMs=0.590  -> PASS
[B] WFS 2->1 reduce   out.mean=0.6000 (expect 0.60 = 1.0*0.5 + 0.5*0.2)  peak=0.6000  launchMs=0.310  -> PASS
[C] IR conv    nodes=4  peak=1.0000  tailPeak=0.8290  segs=16/16  launchMs=0.125  -> PASS
[D] FDN reverb nodes=8  peak=0.0426  tailPeak=0.0223 (feedback tail)  launchMs=0.441  -> PASS
[E] SDN reverb nodes=8  peak=0.0897  tailPeak=0.0897 (coupled tail)  launchMs=0.892  -> PASS
[F] OB 1x1 g=1 d=0    out.mean=0.5000 (expect ~0.50)  peak=0.5000  launchMs=0.148  -> PASS
[G] OB 1->2 scatter   out0=0.5000 (expect 0.50)  out1=0.2500 (expect 0.25)  peak=0.5000  launchMs=0.168  -> PASS
PASS: all GPU backends (WFS gather+reduce, OB scatter, IR conv, FDN, SDN) produced finite, non-silent, ~correct output
```
