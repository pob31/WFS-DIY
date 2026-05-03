#pragma once

/**
    Cross-platform include site for the touch input manager.

    On Linux, this pulls in the real evdev-based EvdevTouchManager.
    On macOS and Windows, it pulls in a no-op stub so MainComponent and
    SystemConfigTab can include the same header on every platform without
    sprinkling #if JUCE_LINUX through them.
*/

#if defined (__linux__)
    #include "Linux/EvdevTouchManager.h"
#else
    #include "TouchManagerStub.h"
#endif
