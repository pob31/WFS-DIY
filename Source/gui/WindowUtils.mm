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

void setFolderIconMac (const char* folderPath)
{
    if (folderPath == nullptr)
        return;

    NSString* path = [NSString stringWithUTF8String:folderPath];

    // Get the app's icon
    NSImage* appIcon = [NSApp applicationIconImage];
    if (appIcon == nil)
        return;

    // Apply the icon to the folder
    [[NSWorkspace sharedWorkspace] setIcon:appIcon forFile:path options:0];
}
#endif
