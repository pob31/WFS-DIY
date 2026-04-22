#pragma once

// Stub JuceHeader.h for CMake builds of this subproject.
//
// The vendored `juce_simpleweb` module was authored for Projucer, which
// auto-generates a global JuceHeader.h on the include path. CMake builds
// don't have that, so we provide this minimal forwarder. It is only placed
// on the include path of targets that link juce_simpleweb (see CMakeLists.txt).

#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
