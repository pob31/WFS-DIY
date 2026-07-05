// SpatcoreAudioCompileCheck.cpp - compile-check + archive anchor for the
// spatcore-audio static library (CMake build only).
//
// Everything in spatcore's audio layers (rt/, dsp/, wfs/, reverb/ and the
// gpu/ host surface) is header-only from this library's point of view: the
// Cuda*/Hip* backend .cpps listed in the target are self-guarding and
// compile to EMPTY translation units under the app-mirroring defines
// (WFS_GPU_NATIVE=1 + WFS_GPU_PLUGINS=1 on Windows/Linux - the vendor code
// is built separately into the dlopen'd per-vendor plugins by
// tools/gpu/build-gpu-plugins.*). On Apple the Metal .mm backends DO carry
// real code, which is why spatcore-audio is a STATIC library with this
// anchor TU rather than an INTERFACE library.
//
// This TU
//   1. guarantees the archive has at least one public symbol on every
//      platform (MSVC's librarian otherwise warns LNK4221 for every empty
//      object and some ar/ranlib implementations reject empty archives);
//   2. compile-checks the public audio header surface under the exact
//      optimization/FP flags in cmake/SpatcoreCompileFlags.cmake, so a
//      header regression fails spatcore's own build instead of the first
//      consumer's.
//
// NOT part of the WFS-DIY app build: the .jucer / vcxproj file lists do not
// reference this file - the app compiles spatcore sources directly through
// Projucer. Vendor-plugin impl headers (Cuda*/Hip*/Metal*Backend.h) and the
// kernel-string headers (*Kernels.h, SHA-256 manifest-locked) are the
// vendor-plugin build surface and are deliberately not included here.

// reverb/ReverbEngine.h uses juce::AudioFormatManager / AudioFormatReader
// (IR file loading) but - verbatim-moved, hygiene pass pending - does not
// include the module header itself; every includer provides it first, as
// the app's TUs did via JuceHeader.h. Do the same here.
#include <juce_audio_formats/juce_audio_formats.h>

// rt/ - lock-free primitives + RT thread utilities
#include "rt/AudioParallelFor.h"
#include "rt/AudioWorkgroupCoordinator.h"
#include "rt/LockFreeRingBuffer.h"
#include "rt/RealtimeThreadUtil.h"
#include "rt/RtThreadPriority.h"
#include "rt/ReverbDiagnostics.h"
#include "rt/RtSnapshot.h"
#include "rt/SharedInputRingBuffer.h"

// dsp/ - smoothers, filters, detectors
#include "dsp/DelayTargetSmoother.h"
#include "dsp/FrDiffusionModel.h"
#include "dsp/InputSpeedLimiter.h"
#include "dsp/LFOWaveforms.h"
#include "dsp/LiveSourceLevelDetector.h"
#include "dsp/NumericGuards.h"
#include "dsp/OutputEQBiquadFilter.h"
#include "dsp/OutputLevelDetector.h"
#include "dsp/ReverbBiquadFilter.h"
#include "dsp/TrackingPositionFilter.h"
#include "dsp/WFSBiquadFilter.h"
#include "dsp/WFSHighShelfFilter.h"

// wfs/ - CPU delay-sum processors + native-GPU renderer wrappers
#include "wfs/InputBufferAlgorithm.h"
#include "wfs/InputBufferProcessor.h"
#include "wfs/OutputBufferAlgorithm.h"
#include "wfs/OutputBufferProcessor.h"
#include "wfs/NativeGpuOutputBufferAlgorithm.h"
#include "wfs/NativeGpuWfsAlgorithm.h"

// reverb/ - ReverbEngine.h pulls the whole algorithm tree (FDN/SDN/IR + GPU
// variants + pre/post processors)
#include "reverb/ReverbEngine.h"
#include "reverb/ReverbFeedThread.h"

// gpu/ - host-facing surface (interfaces, device manager, plugin factory,
// pipeline, host state/configs, compile-time backend selectors)
#include "gpu/GpuBackendInterface.h"
#include "gpu/GpuDeviceManager.h"
#include "gpu/GpuBackendFactory.h"
#include "gpu/GpuAsyncPipeline.h"
#include "gpu/GpuLevelMeters.h"
#include "gpu/PlatformDynLib.h"
#include "gpu/WfsFrHostState.h"
#include "gpu/IrConvHostState.h"
#include "gpu/IrHostFft.h"
#include "gpu/FdnHostConfig.h"
#include "gpu/SdnHostConfig.h"
#include "gpu/WfsGpuBackend.h"
#include "gpu/ObGpuBackend.h"
#include "gpu/IrGpuBackend.h"
#include "gpu/FdnGpuBackend.h"
#include "gpu/SdnGpuBackend.h"

namespace spatcore
{
    // Referenced by nothing at runtime; exists so the archive is never empty.
    int spatcoreAudioCompileCheckAnchor() noexcept { return 1; }
}
