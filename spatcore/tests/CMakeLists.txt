# spatcore unit tests — built only when SPATCORE_STANDALONE_TESTS=ON.
# Dependency-free (no gtest): plain asserts, exit code 0 = pass.
#
# This directory assumes the juce::* targets already exist (spatcore's
# consumer-provides-JUCE contract). Do not configure from here — use
# tests/standalone/CMakeLists.txt, which wires JUCE + juce_simpleweb from the
# WFS-DIY repo's ThirdParty/ and then adds spatcore with tests enabled.

juce_add_console_app(spatcore-tests PRODUCT_NAME "spatcore-tests")

target_sources(spatcore-tests PRIVATE
    SpatcoreTests.cpp)

# The JUCE modules arrive transitively through the libs' PUBLIC link
# interface (their sources compile into this target, JUCE-module style).
target_link_libraries(spatcore-tests PRIVATE
    spatcore-audio
    spatcore-control)

# Same JUCE_WEB_BROWSER/JUCE_USE_CURL trims as the repo's other console
# harness (tools/validation/offline-render).
target_compile_definitions(spatcore-tests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

# Test TUs compile under the same transcribed app flags as the libs.
spatcore_apply_compile_flags(spatcore-tests)
