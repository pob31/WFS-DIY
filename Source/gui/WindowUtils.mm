#include "WindowUtils.h"

#if JUCE_MAC
#import <Cocoa/Cocoa.h>

void enableDarkTitleBarMac(void* nsWindow)
{
    if (nsWindow == nullptr)
        return;

    NSWindow* window = (__bridge NSWindow*)nsWindow;

    // Set the window appearance to dark Aqua (Mojave+)
    if (@available(macOS 10.14, *))
    {
        window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    }
}
#endif
