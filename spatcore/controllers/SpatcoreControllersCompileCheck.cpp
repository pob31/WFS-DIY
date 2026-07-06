// SpatcoreControllersCompileCheck.cpp - compile-check + archive anchor for
// the spatcore-controllers static library (CMake build only).
//
// The controller device layer is header-only apart from the Linux evdev
// touch .cpps listed in the target, which self-guard on __linux__ and
// compile to EMPTY translation units elsewhere (same pattern as the gpu
// backend .cpps in spatcore-audio). This TU
//   1. guarantees the archive has at least one public symbol on every
//      platform (MSVC librarian LNK4221 / empty-ar hygiene);
//   2. compile-checks the public controller header surface under the exact
//      flags in cmake/SpatcoreCompileFlags.cmake, so a header regression
//      fails spatcore's own build instead of the first consumer's.
//
// NOT part of the WFS-DIY app build: the .jucer / vcxproj file lists do not
// reference this file - the app compiles spatcore sources directly through
// Projucer.
//
// hidapi note: StreamDeckDevice / XencelabsDevice / SpaceMouseDevice include
// "hidapi/hidapi.h" and call hid_* only from implicitly-inline member
// functions, so compiling (and even linking) this TU needs hidapi HEADERS
// only (SPATCORE_HIDAPI_INCLUDE_DIR); the hid_* implementation is provided
// by the final application (WFS-DIY compiles ThirdParty/hidapi/
// HidApiPlatform.c).

// Generic controller abstractions (thread+timer polling, hotplug, events,
// axis/button mapping model)
#include "ControllerEvent.h"
#include "ControllerDevice.h"
#include "ControllerMapping.h"

// 3DConnexion SpaceMouse 6DOF HID driver
#include "spacemouse/SpaceMouseDevice.h"

// Elgato Stream Deck+ HID driver + page/binding model + framebuffer renderer
#include "streamdeck/StreamDeckDevice.h"
#include "streamdeck/StreamDeckPage.h"
#include "streamdeck/StreamDeckRenderer.h"

// Xencelabs Quick Keys HID driver + wheel page model
#include "xencelabs/XencelabsDevice.h"
#include "xencelabs/QuickKeysPage.h"

// ROLI Lightpad BLOCKS (roli_blocks_basics JUCE module)
#include "lightpad/LightpadTypes.h"
#include "lightpad/LightpadDevice.h"

// Touchscreen input (Linux evdev implementation behind a cross-platform
// include site; the stub compiles on Windows/macOS)
#include "touch/TouchDeviceMapping.h"
#include "touch/TouchManager.h"

namespace spatcore
{
    // Referenced by nothing at runtime; exists so the archive is never empty.
    int spatcoreControllersCompileCheckAnchor() noexcept { return 1; }
}
