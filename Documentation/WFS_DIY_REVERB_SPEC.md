# WFS-DIY Reverb Algorithm — Design Specification

## Context

The WFS-DIY JUCE application has a Reverb tab with infrastructure for 0–16 reverb nodes. The Reverb tab has three sub-tabs:

1. **Channel Parameters** (implemented)
2. **Reverb EQ** (content defined here)
3. **Algorithm** (content defined here)

The feed/return routing is handled by the existing system. This spec covers the reverb DSP between feed and return, and the content of the EQ and Algorithm sub-tabs.

### References

- De Sena et al. (2015), "Efficient synthesis of room acoustics via scattering delay networks"
- Guesney, M. (2020), "Synthèse du champ réverbéré en Audio Orienté Objet", ENS Louis-Lumière
- Jot, J.-M. (1991), "Digital delay networks for designing artificial reverberators"

### Constants

```cpp
static constexpr float SPEED_OF_SOUND = 343.0f; // m/s
static constexpr int MAX_REVERB_NODES = 16;
static constexpr int MAX_DELAY_SAMPLES = 8192;  // ~170ms at 48kHz
```

---

## Global vs Per-Node Parameters

Almost everything is **global**. The user sets up the reverb behaviour once. Nodes differ only in spatial placement and aiming.

### Per-Node

| Parameter           | Sub-tab            | Reason                          |
|---------------------|--------------------|---------------------------------|
| Name                | Channel Parameters | Identity                        |
| Position X/Y/Z      | Channel Parameters | Spatial placement               |
| Return Offset X/Y/Z | Channel Parameters | Output position offset          |
| Orientation (°)     | Channel Parameters | Directivity aiming (horizontal) |
| Pitch (°)           | Channel Parameters | Directivity aiming (vertical)   |

### Global

All other Channel Parameters, all Reverb EQ settings, all Algorithm settings:

| Parameter                              | Sub-tab            |
|----------------------------------------|--------------------|
| Attenuation (dB)                       | Channel Parameters |
| Delay/Latency (ms)                     | Channel Parameters |
| Angle On / Angle Off (directivity)     | Channel Parameters |
| HF Damping (dB/m)                      | Channel Parameters |
| Distance Atten %                       | Channel Parameters |
| Minimal Latency toggle                 | Channel Parameters |
| Live Source Atten toggle               | Channel Parameters |
| Distance Atten (dB/m) — return         | Channel Parameters |
| Common Atten (%) — return              | Channel Parameters |
| Mute Macro — return                    | Channel Parameters |
| Output Mutes (1–16) — return           | Channel Parameters |
| Pre-EQ, Post-EQ                        | Reverb EQ          |
| Pre-Compressor, Post-Expander          | Reverb EQ          |
| Algorithm (SDN/FDN/IR)                 | Algorithm          |
| RT60, crossovers, diffusion            | Algorithm          |
| Scale (SDN) / Size (FDN) / IR settings | Algorithm          |
| Wet Level                              | Algorithm          |

---

## Processing Chain

Global pre/post processing. Per-node instances share the same settings.

```
Per-node feed from existing infrastructure
(summed from inputs, distance-attenuated, delayed, HF-damped,
directivity-filtered per Channel Parameters)
    │
    ▼
╔═══════════════════════════════════════════════╗
║  GLOBAL PRE-PROCESSING (shared settings)      ║
║                                               ║
║  Pre-EQ                                       ║
║    │                                          ║
║    ├── sidechain tap (post-EQ dry signal) ──┐ ║
║    │                                        │ ║
║    ▼                                        │ ║
║  Pre-Compressor                             │ ║
╚═════════════════════════════════════════════│═╝
    │                                         │
    ▼                                         │
┌───────────────────────────────────────┐     │
│  Reverb Algorithm (SDN / FDN / IR)    │     │
│  Per-node instances, global settings  │     │
└───────────────────────────────────────┘     │
    │                                         │
    ▼                                         │
╔═════════════════════════════════════════════│═╗
║  GLOBAL POST-PROCESSING (shared settings)   │ ║
║                                             │ ║
║  Post-EQ                                    │ ║
║    │                                        │ ║
║    ▼                                        │ ║
║  Post-Expander ◄──── sidechain key ────────┘ ║
╚═══════════════════════════════════════════════╝
    │
    ▼
Per-node return to WFS output matrix
(via return infrastructure: offset position,
distance atten, output mutes)
```

**Pre-Compressor**: tames peaks before they hit the reverb. Prevents disproportionate reverb bursts from transients.

**Post-Expander**: keyed on post-pre-EQ dry signal. Gently reduces the exposed reverb tail when the source goes quiet. Prevents the naked tail from sounding artificial with nothing to blend into.

**Sidechain tap**: after pre-EQ, before compressor. The expander tracks the same spectral content feeding the reverb, with unwanted frequencies already filtered out.

---

## Reverb EQ Sub-Tab

```
┌─ Reverb EQ ─────────────────────────────────────────────────┐
│                                                             │
│  ── Pre-Processing ─────────────────────────────────────    │
│                                                             │
│  Pre-EQ:                                     [Bypass]       │
│    HPF: [====] 80 Hz      Slope: [12 dB/oct ▼]             │
│    LPF: [====] 12 kHz     Slope: [12 dB/oct ▼]             │
│    Band 1: [freq] [gain dB] [Q]                             │
│    Band 2: [freq] [gain dB] [Q]                             │
│                                                             │
│  Pre-Compressor:                             [Bypass]       │
│    Threshold: [====] -12 dB                                 │
│    Ratio:     [====] 2:1                                    │
│    Attack:    [====] 10 ms                                  │
│    Release:   [====] 100 ms                                 │
│                                                             │
│  ── Post-Processing ────────────────────────────────────    │
│                                                             │
│  Post-EQ:                                    [Bypass]       │
│    HPF: [====] off                                          │
│    LPF: [====] off                                          │
│    Band 1: [freq] [gain dB] [Q]                             │
│    Band 2: [freq] [gain dB] [Q]                             │
│                                                             │
│  Post-Expander (keyed on post-EQ dry):       [Bypass]       │
│    Threshold: [====] -40 dB                                 │
│    Ratio:     [====] 1:2                                    │
│    Attack:    [====] 1 ms                                   │
│    Release:   [====] 200 ms                                 │
│                                                             │
│  ── Visualisation ──────────────────────────────────────    │
│                                                             │
│  [Combined pre+post EQ frequency response curve]            │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### EQ Parameters

Each EQ (pre and post):
- HPF: 20–1000 Hz, slope 6/12/18/24 dB/oct. Off at minimum.
- LPF: 1–20 kHz, slope 6/12/18/24 dB/oct. Off at maximum.
- 2 parametric bands: freq 20 Hz–20 kHz, gain ±18 dB, Q 0.1–10
- Bypass toggle

### Dynamics Parameters

Pre-Compressor:
- Threshold: -60 to 0 dB
- Ratio: 1:1 to 20:1
- Attack: 0.1–100 ms
- Release: 10–1000 ms
- Bypass toggle (default: bypassed)

Post-Expander:
- Key: post-pre-EQ dry signal (per node)
- Threshold: -80 to -10 dB
- Ratio: 1:1 to 1:8
- Attack: 0.1–50 ms
- Release: 50–2000 ms
- Bypass toggle (default: bypassed)

---

## Algorithm Sub-Tab

```
┌─ Algorithm ─────────────────────────────────────────────────┐
│                                                             │
│  Algorithm: [SDN] [FDN] [IR]         (toggle selector)      │
│                                                             │
│  ── Decay ──────────────────── (SDN & FDN only) ──────────  │
│                                                             │
│  RT60:            [=======|=======] 1.5 s                   │
│  RT60 Low ×:      [=======|=======] 1.3                     │
│  RT60 High ×:     [=======|=======] 0.5                     │
│  Crossover Low:   [=======|=======] 200 Hz                  │
│  Crossover High:  [=======|=======] 4000 Hz                 │
│  Diffusion:       [=======|=======] 0.5                     │
│                                                             │
│  ── SDN ────────────────── (visible when SDN) ────────────  │
│                                                             │
│  Scale:           [=======|=======] 1.0                     │
│                                                             │
│  ── FDN ────────────────── (visible when FDN) ────────────  │
│                                                             │
│  Size:            [=======|=======] 1.0                     │
│                                                             │
│  ── IR ─────────────────── (visible when IR) ─────────────  │
│                                                             │
│  IR File:    [Load...] "hall_medium.wav"      [▶ Preview]   │
│  IR Trim:    [=======|=======] 0 ms                         │
│  IR Length:  [=======|=======] full                          │
│  Per-node:   [Off / On]                                     │
│                                                             │
│  ── Output ─────────────────────────────────────────────    │
│                                                             │
│  Wet Level:  [=======|=======] 0.0 dB                       │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

Decay parameters greyed out when IR is selected. Scale visible only for SDN. Size visible only for FDN. IR controls visible only for IR.

### Algorithm Parameter Ranges

| Parameter      | Range          | Default | Applies to | Description                                          |
|----------------|----------------|---------|------------|------------------------------------------------------|
| algorithm      | SDN / FDN / IR | SDN     | All        | Active reverb algorithm                              |
| rt60           | 0.2–8.0 s      | 1.5     | SDN, FDN   | Mid-frequency decay time                             |
| rt60LowMult    | 0.1–9.0 ×      | 1.3     | SDN, FDN   | LF decay = rt60 × this multiplier                    |
| rt60HighMult   | 0.1–9.0 ×      | 0.5     | SDN, FDN   | HF decay = rt60 × this multiplier                    |
| crossoverLow   | 50–500 Hz      | 200     | SDN, FDN   | Low/mid crossover for decay bands                    |
| crossoverHigh  | 1000–10000 Hz  | 4000    | SDN, FDN   | Mid/high crossover for decay bands                   |
| diffusion      | 0.0–1.0        | 0.5     | SDN, FDN   | Allpass diffusion amount                             |
| scale          | 0.5–4.0        | 1.0     | SDN        | Multiplier on inter-node delays                      |
| size           | 0.5–2.0        | 1.0     | FDN        | Scales internal delay line lengths                   |
| irFile         | file path      | —       | IR         | IR file to load                                      |
| irTrim         | 0–100 ms       | 0       | IR         | Skip direct sound at start of IR                     |
| irLength       | 0.1–6.0 s      | full    | IR         | Truncate IR tail (saves CPU)                         |
| perNodeIR      | on / off       | off     | IR         | Load different IR per node                           |
| wetLevel       | -inf to +12 dB | 0.0     | All        | Output level                                         |

---

## Algorithm 1: SDN (Scattering Delay Network)

### Concept

Nodes interconnected via delay lines. Delays and gains derived from physical distances between node positions. Each node scatters incoming energy to all other nodes using a Householder matrix. The result is a coherent reverberant field where the spatial relationships are physically meaningful.

N active nodes → N(N-1) inter-node delay lines.

### Processing (per sample or per block)

```
For each active node n:

  1. nodeInput = signal from global pre-processing output

  2. Read incoming signals from all other nodes:
     incoming[i] = interNodeDelay[i→n].read()

  3. Householder scattering:
     X = (2 / (N-1)) × sum(all incoming)
     scattered_to[i] = X - incoming[i]   for each other node i

  4. For each outgoing path to node i:
     signal = scattered_to[i] + nodeInput × inputDistribution
     signal = decayFilter[n→i].process(signal)
     signal = diffusionAllpass[n].process(signal)
     interNodeDelay[n→i].write(signal)

  5. nodeOutput = sum(scattered_to[all]) × wetLevel
     → feeds into global post-processing
```

### Scattering Matrix (Householder Isotropic)

```
A = (2/(N-1)) × J - I
```

where J = all-ones matrix, I = identity, dimension (N-1)×(N-1).

Efficient computation:
```
X = (2/(N-1)) × (x_0 + x_1 + ... + x_{N-2})   // compute once
output_i = X - x_i                               // per output
```
Cost: 1 multiplication + 2N-1 additions (vs N² multiplications for naive matrix).

### Inter-Node Delays

```
delay_samples[a→b] = dist(pos_a, pos_b) / SPEED_OF_SOUND × sampleRate × scale
```

Positions from the per-node Position X/Y/Z fields. Crossfade on position changes (~10ms ramp). Max buffer: 8192 samples.

### Frequency-Dependent Decay

Per inter-node path, 3-band crossover with per-band gain:

```
delay_sec = (dist(a, b) × scale) / SPEED_OF_SOUND

rt60_low  = rt60 × rt60LowMult
rt60_mid  = rt60
rt60_high = rt60 × rt60HighMult

gain_band = pow(0.001, delay_sec / rt60_band)
```

Two one-pole crossover filters at crossoverLow and crossoverHigh:
```
coeff = 1.0 - exp(-2π × freq / sampleRate)
```

Band split: lowState and highState track the energy, mid = input - low - high.

### Diffusion

2 allpass filters per node on summed input before scattering:
- Delays: 142 and 277 samples (at 48kHz, scale proportionally at other rates)
- Coefficient: `diffusion × 0.5`
- Bypassed when diffusion = 0

SDN already provides good density via multiple scattering — heavy diffusion risks metallic flutter on transients. Keep conservative.

### Resource Scaling

| Nodes | Inter-node lines | Notes                          |
|-------|-----------------|--------------------------------|
| 4     | 12              | Light                          |
| 6     | 30              | Typical frontal config         |
| 8     | 56              | Moderate                       |
| 12    | 132             | Heavy — consider FDN           |
| 16    | 240             | Very heavy — FDN recommended   |

### Node Placement

Avoid symmetric or equidistant arrangements — they create modal resonances. Break symmetry. Typical frontal-6: 2 left, 2 right, 2 upstage at varied depths.

---

## Algorithm 2: FDN (Feedback Delay Network)

### Concept

Each node runs an independent 16-line FDN. No inter-node coupling. Spatial distribution comes entirely from the existing feed/return infrastructure. Simpler, lighter, very tuneable.

### Processing per Node

```
  1. input = signal from global pre-processing output

  2. diffused = cascadeAllpass(input, 4 stages)
     // delays: 142, 107, 379, 277 samples at 48kHz
     // coefficient: diffusion × 0.7

  3. delayOut[0..15] = read from 16 internal delay lines

  4. mixed[0..15] = hadamardTransform(delayOut)
     mixed *= 0.25   // 1/sqrt(16) energy preservation

  5. For line 0..15:
       decayed = decayFilter[line].process(mixed[line])
       delayLine[line].write(decayed + diffused × inputGain[line])

  6. output = sum(delayOut[line] × outputTapSign[line])
     output = dcBlock(output)      // y = x - x_prev + 0.9995 × y_prev
     output × wetLevel
     → feeds into global post-processing
```

### Internal Delay Lines

16 per node, mutually prime lengths at 48kHz:

```
Short (early density):     509,  571,  631,  701
Medium:                    797,  887,  967, 1061
Long (modal density):     1151, 1259, 1373, 1481
Very long (LF support):   1601, 1733, 1867, 1997
```

All prime. Scaled by `size` parameter. Max buffer: 8192 samples.

Per-node decorrelation: offset base delays by `nodeIndex × 6` samples.

### Hadamard Transform

Fast Walsh-Hadamard, 16-point, in-place:

```cpp
for (int len = 1; len < 16; len <<= 1)
    for (int i = 0; i < 16; i += len << 1)
        for (int j = i; j < i + len; ++j) {
            float a = out[j], b = out[j + len];
            out[j]       = a + b;
            out[j + len] = a - b;
        }
// Scale all by 0.25
```

### Decay per Delay Line

```
delay_sec = (baseDelay[line] × size) / sampleRate

gain_low  = pow(0.001, delay_sec / (rt60 × rt60LowMult))
gain_mid  = pow(0.001, delay_sec / rt60)
gain_high = pow(0.001, delay_sec / (rt60 × rt60HighMult))
```

Same 3-band crossover as SDN.

### Input/Output Gains

Input distribution to delay lines: slight gain variation around 1/16 (±1%) for decorrelation.

Output taps: alternating sign pattern [+, -, +, -, ...] at magnitude 1/4 for decorrelation.

### DC Blocker

```
y[n] = x[n] - x[n-1] + 0.9995 × y[n-1]
```

### Resource Usage

~512KB per node (16 × 8192 × 4 bytes). Constant regardless of node count.
16 nodes = ~8MB total.

---

## Algorithm 3: IR (Impulse Response Convolution)

### Concept

Each node convolves its input with a loaded impulse response. Maximum realism from captured spaces. Validated approach from the Max prototype with Altiverb.

Uses `juce::dsp::Convolution` (partitioned convolution — small partitions for low latency on early part, large partitions for efficient tail processing).

### Processing per Node

```
  1. input = signal from global pre-processing output
  2. output = convolver.process(input)
  3. output × wetLevel
     → feeds into global post-processing
```

### IR Loading

```cpp
juce::dsp::Convolution convolver;
convolver.loadImpulseResponse(
    irFile,
    juce::dsp::Convolution::Stereo::no,
    juce::dsp::Convolution::Trim::yes,
    irTrimSamples);
```

### Modes

- **Global IR** (default): one file, all nodes share it
- **Per-node IR**: each node loads a different file (when perNodeIR is on)

### Notes

- Decay parameters (RT60, crossovers, diffusion) are **greyed out** in IR mode — the IR defines its own character
- Pre/post EQ and dynamics remain available
- User changes sonic character primarily by selecting different IR files
- IR Trim: skip the direct sound portion at the start (the WFS system already handles direct path)
- IR Length: truncate the tail to save CPU

### IR Sourcing

Users will typically:
- Use IRs from existing collections (Altiverb library, OpenAIR, etc.)
- Capture their own IRs on location
- Use synthetic IRs from acoustic modelling software
- Eventually: use IRs from the future venue calibration system

The engine should accept standard mono WAV/AIFF files.

### CPU Notes

Convolution cost scales with IR length:

| IR Length | CPU      | Use case                          |
|-----------|----------|-----------------------------------|
| 0.5 s     | Low      | Small rooms, high node counts     |
| 1.5 s     | Moderate | Typical hall                      |
| 3.0 s     | High     | Large hall / church               |
| 6.0 s     | Very high| Cathedral (reduce node count)     |

---

## Algorithm Comparison

| Criterion                  | SDN                         | FDN                        | IR                          |
|----------------------------|-----------------------------|----------------------------|-----------------------------|
| Spatial coherence          | Excellent — interconnected  | Moderate — independent     | Depends on IR               |
| Physical accuracy          | High — geometry-driven      | Low — arbitrary delays     | High — captured reality     |
| Toneability                | Moderate — tied to geometry | High — free parameters     | Low — change IR file        |
| CPU (6 nodes)              | Moderate                    | Low                        | Moderate–High               |
| CPU (16 nodes)             | Very high (240 lines)       | Moderate (256 lines)       | Very high                   |
| Latency                    | None (sample-by-sample)     | None (sample-by-sample)    | Partition-dependent         |
| Setup effort               | Place nodes, set RT60       | Set RT60 and size          | Find/load right IR          |
| Node movement response     | Reverb changes naturally    | No change in tone          | No change in tone           |
| Best for                   | Spatial accuracy, intimate  | Quick setup, many nodes    | Known spaces, realism       |

---

## Implementation Notes

### Class Structure

```
ReverbEngine
│
├── GlobalPreProcessor
│   ├── PreEQ (HPF + LPF + 2 parametric bands)
│   └── PreCompressor
│
├── ReverbAlgorithm (abstract interface)
│   ├── SDNAlgorithm
│   │   └── inter-node delay lines + scattering + decay filters
│   ├── FDNAlgorithm
│   │   └── per-node: 16 delay lines + Hadamard + decay filters
│   └── IRAlgorithm
│       └── per-node: juce::dsp::Convolution instance
│
├── GlobalPostProcessor
│   ├── PostEQ (HPF + LPF + 2 parametric bands)
│   └── PostExpander (with sidechain input from PreEQ output)
│
└── Parameters (all global except positions/orientations)
```

### Abstract Algorithm Interface

```cpp
class ReverbAlgorithm
{
public:
    virtual ~ReverbAlgorithm() = default;

    virtual void prepare (double sampleRate, int maxBlockSize, int numNodes) = 0;
    virtual void reset() = 0;

    // nodeInputs: one channel per active node (pre-processed)
    // nodeOutputs: one channel per active node
    virtual void processBlock (const juce::AudioBuffer<float>& nodeInputs,
                               juce::AudioBuffer<float>& nodeOutputs,
                               int numSamples) = 0;

    virtual void setParameters (const AlgorithmParameters& params) = 0;

    // Called when node count or positions change
    virtual void updateGeometry (const std::vector<NodePosition>& nodes) = 0;
};
```

### Algorithm Switching

When the user changes algorithm:
1. Prepare the new algorithm instance
2. Crossfade output from old to new over ~50ms
3. Release old instance
No audio interruption.

### Performance

- The reverb path is not latency-critical. Can accumulate into larger blocks (256–512 samples) internally.
- At 96kHz host rate, consider processing at 48kHz internally (decimate/interpolate). Late reverb has negligible energy above 10kHz.
- Parameter changes smoothed or applied at block boundaries to avoid clicks.
- Node position changes: crossfade delay lengths over ~10ms.
