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

//==============================================================================
// Real-time audio: opt the whole process out of App Nap and timer coalescing
// for the app's lifetime. Without this, macOS lowers the process QoS when the
// WFS-DIY window is not frontmost (e.g. the operator switches to QLab mid-show),
// and the DSP worker threads drift off the performance cores onto the efficiency
// cores. This is the macOS counterpart to the Windows HIGH_PRIORITY_CLASS +
// EcoQoS opt-out in Main.cpp.
//
// NSActivityUserInitiated  -> exempt from App Nap (the actual fix).
// NSActivityLatencyCritical -> also disable timer coalescing for the 50 Hz
//                              control timer and audio worker polling.
// UserInitiated additionally holds off *idle system sleep* while the app is open
// (desirable for live use). To allow idle sleep instead, swap UserInitiated for
// NSActivityUserInitiatedAllowingIdleSystemSleep.
static id<NSObject> gRealtimeAudioActivityToken = nil;

void beginRealtimeAudioActivityMac()
{
    if (gRealtimeAudioActivityToken != nil)
        return; // already active

    id<NSObject> token = [[NSProcessInfo processInfo]
        beginActivityWithOptions: (NSActivityUserInitiated | NSActivityLatencyCritical)
                          reason: @"WFS-DIY realtime audio processing"];

   #if __has_feature(objc_arc)
    gRealtimeAudioActivityToken = token;            // ARC: strong static retains it
   #else
    gRealtimeAudioActivityToken = [token retain];   // MRC: keep alive for app lifetime
   #endif
}

void endRealtimeAudioActivityMac()
{
    if (gRealtimeAudioActivityToken == nil)
        return;

    [[NSProcessInfo processInfo] endActivity: gRealtimeAudioActivityToken];

   #if !__has_feature(objc_arc)
    [gRealtimeAudioActivityToken release];
   #endif
    gRealtimeAudioActivityToken = nil;
}
#endif
