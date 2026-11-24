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

//==============================================================================
// Full screen that stays on the current Space (see WindowUtils::installFakeFullScreen).
//
// JUCE gives every resizable window with a maximise button the collection
// behaviour FullScreenPrimary, so the green button sends toggleFullScreen: and
// AppKit moves the window to a Space of its own. The JUCE window class
// overrides toggleFullScreen: and calls NSWindow's version through super, so
// replacing NSWindow's implementation catches the green button, the system
// full-screen shortcut and ResizableWindow::setFullScreen for every JUCE
// window without patching JUCE. Windows that are not JUCE's (open panels,
// alerts) and a window already in native full screen keep AppKit's behaviour.
//
// Full screen here is: frame = the display's whole frame, title bar kept (its
// green button is the way back for a mouse or a finger), size pinned and the
// window not movable, menu bar and Dock hidden while any such window sits on a
// display that has a menu bar. Presentation options belong to the app, not to
// a display.
#import <objc/runtime.h>
#include <map>
#include <memory>
#include <vector>

namespace
{
    IMP gOriginalToggleFullScreen = nullptr;
    int gMenuBarHiders = 0;

    /** Leaves full screen when JUCE hides the window (setVisible (false) is an
        orderOut:, not a close), so a hidden window cannot keep the menu bar away. */
    struct FakeFullScreenVisibilityListener : juce::ComponentListener
    {
        FakeFullScreenVisibilityListener (juce::Component& c, NSWindow* w) : component (&c), window (w)
        {
            c.addComponentListener (this);
        }

        ~FakeFullScreenVisibilityListener() override
        {
            if (auto* c = component.getComponent())
                c->removeComponentListener (this);
        }

        void componentVisibilityChanged (juce::Component& c) override;

        juce::Component::SafePointer<juce::Component> component;
        NSWindow* window;   // not retained: the window close observer drops the entry first
    };

    struct FakeFullScreenState
    {
        NSRect frame {};
        NSWindowStyleMask styleMask = 0;
        juce::Component::SafePointer<juce::Component> component;
        bool hadLimits = false;
        int minW = 0, minH = 0, maxW = 0, maxH = 0;
        bool hidesMenuBar = false;
        bool wasMovable = true;
        std::unique_ptr<FakeFullScreenVisibilityListener> visibilityListener;
    };

    std::map<void*, FakeFullScreenState>& fakeFullScreenWindows()
    {
        static std::map<void*, FakeFullScreenState> windows;
        return windows;
    }

    void* keyFor (NSWindow* window)   { return (__bridge void*) window; }

    juce::Component* findJuceWindow (NSWindow* window)
    {
        if (window == nil)
            return nullptr;

        auto& desktop = juce::Desktop::getInstance();

        for (int i = desktop.getNumComponents(); --i >= 0;)
            if (auto* c = desktop.getComponent (i))
                if (auto* peer = c->getPeer())
                    if (auto* view = (__bridge NSView*) peer->getNativeHandle(); view != nil && [view window] == window)
                        return c;

        return nullptr;
    }

    bool screenHasMenuBar (NSScreen* screen)
    {
        return screen != nil && ([NSScreen screensHaveSeparateSpaces] || screen == [[NSScreen screens] firstObject]);
    }

    /** The options in force before the first window hid the menu bar (the
        user's auto-hide, for one), given back when the last one leaves. */
    NSApplicationPresentationOptions gPresentationBeforeHiding = NSApplicationPresentationDefault;

    void changeMenuBarHiders (int delta)
    {
        const auto before = gMenuBarHiders;
        gMenuBarHiders = juce::jmax (0, gMenuBarHiders + delta);

        if (before == 0 && gMenuBarHiders > 0)
        {
            gPresentationBeforeHiding = [NSApp presentationOptions];
            [NSApp setPresentationOptions: NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar];
        }
        else if (before > 0 && gMenuBarHiders == 0)
        {
            // The user's auto-hide reads back as AutoHideMenuBar alone, a combination
            // AppKit documents as invalid (it may raise): Default shows it again.
            @try
            {
                [NSApp setPresentationOptions: gPresentationBeforeHiding];
            }
            @catch (NSException*)
            {
                [NSApp setPresentationOptions: NSApplicationPresentationDefault];
            }
        }
    }

    /** Pins JUCE's constrainer to the content of a window covering the display:
        its usual maximum is the display's user area (fitResizeLimitsToDisplay),
        which leaves out the menu bar and Dock, and an edge drag must not
        resize a full-screen window. JUCE's limits are content sizes, not frames. */
    void pinResizeLimits (FakeFullScreenState& s, NSWindow* window, NSRect target)
    {
        if (! s.hadLimits)
            return;

        const NSSize content = [window contentRectForFrameRect: target].size;
        const int w = (int) std::ceil (content.width), h = (int) std::ceil (content.height);

        if (auto* rw = dynamic_cast<juce::ResizableWindow*> (s.component.getComponent()))
            rw->setResizeLimits (w, h, w, h);
    }

    void enterFakeFullScreen (NSWindow* window, juce::Component* component)
    {
        NSScreen* screen = [window screen] != nil ? [window screen] : [NSScreen mainScreen];

        if (screen == nil)
            return;

        const NSRect target = [screen frame];

        FakeFullScreenState s;
        s.frame = [window frame];
        s.styleMask = [window styleMask];
        s.component = component;
        s.hidesMenuBar = screenHasMenuBar (screen);

        if (auto* rw = dynamic_cast<juce::ResizableWindow*> (component))
        {
            if (auto* constrainer = rw->getConstrainer())
            {
                s.hadLimits = true;
                s.minW = constrainer->getMinimumWidth();
                s.minH = constrainer->getMinimumHeight();
                s.maxW = constrainer->getMaximumWidth();
                s.maxH = constrainer->getMaximumHeight();
            }
        }

        if (component != nullptr)
            s.visibilityListener = std::make_unique<FakeFullScreenVisibilityListener> (*component, window);

        // In the map before any resize: moved() -> fitResizeLimitsToDisplay checks it
        auto& entry = fakeFullScreenWindows()[keyFor (window)];
        entry = std::move (s);
        entry.wasMovable = [window isMovable];
        pinResizeLimits (entry, window, target);

        if (entry.hidesMenuBar)
            changeMenuBarHiders (+1);

        // The title bar stays, with its green button: the way back out for a
        // mouse or a finger. It sits where the hidden menu bar was. The window
        // must not be dragged off the display while it covers it.
        [window setMovable: NO];
        [window setFrame: target display: YES];
        [window makeKeyAndOrderFront: nil];
    }

    /** restore = false when the window is closing: only the bookkeeping goes. */
    void exitFakeFullScreen (NSWindow* window, bool restore)
    {
        auto& windows = fakeFullScreenWindows();
        auto it = windows.find (keyFor (window));

        if (it == windows.end())
            return;

        // Out of the map first: the moved() the frame change sends refits limits again.
        // The visibility listener may be the caller: it is destroyed at the end of
        // this function and touches nothing of its own after that.
        FakeFullScreenState s = std::move (it->second);
        windows.erase (it);

        if (s.hidesMenuBar)
            changeMenuBarHiders (-1);

        if (! restore)
            return;

        // Limits before the frame: the frame change's moved() keeps the constrainer's
        // minimum (fitResizeLimitsToDisplay), still pinned to the display's size.
        if (s.hadLimits)
            if (auto* rw = dynamic_cast<juce::ResizableWindow*> (s.component.getComponent()))
                rw->setResizeLimits (s.minW, s.minH, s.maxW, s.maxH);

        [window setStyleMask: s.styleMask];
        [window setMovable: s.wasMovable];
        [window setFrame: s.frame display: YES];
    }

    void FakeFullScreenVisibilityListener::componentVisibilityChanged (juce::Component& c)
    {
        if (! c.isVisible())
            exitFakeFullScreen (window, true);   // destroys this listener; return straight away
    }

    bool offersFullScreen (NSWindow* window)
    {
        return ([window collectionBehavior] & NSWindowCollectionBehaviorFullScreenPrimary) != 0;
    }

    /** Toggles our full screen; false when the window is not one we handle. */
    bool toggleFakeFullScreen (NSWindow* window)
    {
        if (fakeFullScreenWindows().count (keyFor (window)) != 0)
        {
            exitFakeFullScreen (window, true);
            return true;
        }

        if (([window styleMask] & NSWindowStyleMaskFullScreen) != 0)
            return false;   // already in native full screen: let AppKit bring it back

        auto* component = findJuceWindow (window);

        if (component == nullptr)
            return false;

        enterFakeFullScreen (window, component);
        return true;
    }

    IMP gOriginalMiniaturize = nullptr;
    IMP gOriginalPerformMiniaturize = nullptr;

    /** A minimised window must not keep the menu bar hidden or come back
        pinned over the display: leave full screen first, then minimise as a
        window (a borderless window could not be minimised at all). */
    void replacementMiniaturize (id self, SEL cmd, id sender)
    {
        exitFakeFullScreen ((NSWindow*) self, true);

        if (auto original = (cmd == @selector (performMiniaturize:) ? gOriginalPerformMiniaturize : gOriginalMiniaturize))
            ((void (*) (id, SEL, id)) original) (self, cmd, sender);
    }

    void replacementToggleFullScreen (id self, SEL cmd, id sender)
    {
        if (toggleFakeFullScreen ((NSWindow*) self))
            return;

        if (gOriginalToggleFullScreen != nullptr)
            ((void (*) (id, SEL, id)) gOriginalToggleFullScreen) (self, cmd, sender);
    }

    /** A window that moved to another display, or whose display changed size or went away. */
    void refitFakeFullScreenWindows()
    {
        std::vector<void*> keys;

        for (auto& [key, state] : fakeFullScreenWindows())
            keys.push_back (key);

        for (auto* key : keys)
        {
            auto* window = (__bridge NSWindow*) key;
            NSScreen* screen = [window screen];

            if (screen == nil)
            {
                exitFakeFullScreen (window, true);
                continue;
            }

            auto& s = fakeFullScreenWindows()[key];
            const bool hides = screenHasMenuBar (screen);

            if (hides != s.hidesMenuBar)
            {
                s.hidesMenuBar = hides;
                changeMenuBarHiders (hides ? +1 : -1);
            }

            pinResizeLimits (s, window, [screen frame]);
            [window setFrame: [screen frame] display: YES];
        }
    }
}

void installFakeFullScreenMac()
{
    if (gOriginalToggleFullScreen != nullptr)
        return;

    Method method = class_getInstanceMethod ([NSWindow class], @selector (toggleFullScreen:));

    if (method == nullptr)
        return;

    gOriginalToggleFullScreen = method_setImplementation (method, (IMP) replacementToggleFullScreen);

    if (Method m = class_getInstanceMethod ([NSWindow class], @selector (miniaturize:)))
        gOriginalMiniaturize = method_setImplementation (m, (IMP) replacementMiniaturize);

    if (Method m = class_getInstanceMethod ([NSWindow class], @selector (performMiniaturize:)))
        gOriginalPerformMiniaturize = method_setImplementation (m, (IMP) replacementMiniaturize);

    // Ctrl+Cmd+F: the app has no View menu, so AppKit adds no Enter Full Screen
    // item and the shortcut would otherwise do nothing.
    [NSEvent addLocalMonitorForEventsMatchingMask: NSEventMaskKeyDown
                                          handler: ^NSEvent* (NSEvent* event)
    {
        const auto mods = [event modifierFlags] & (NSEventModifierFlagShift | NSEventModifierFlagControl
                                                   | NSEventModifierFlagOption | NSEventModifierFlagCommand);

        if (mods == (NSEventModifierFlagControl | NSEventModifierFlagCommand)
            && [[event charactersIgnoringModifiers] caseInsensitiveCompare: @"f"] == NSOrderedSame)
        {
            NSWindow* window = [event window] != nil ? [event window] : [NSApp keyWindow];

            if (window != nil
                && (offersFullScreen (window) || fakeFullScreenWindows().count (keyFor (window)) != 0)
                && toggleFakeFullScreen (window))
                return nil;
        }

        return event;
    }];

    auto* centre = [NSNotificationCenter defaultCenter];

    [centre addObserverForName: NSWindowWillCloseNotification
                        object: nil
                         queue: nil
                    usingBlock: ^(NSNotification* note)
    {
        exitFakeFullScreen ((NSWindow*) [note object], false);
    }];

    // A minimised window leaves full screen: it would keep the menu bar hidden
    [centre addObserverForName: NSWindowDidMiniaturizeNotification
                        object: nil
                         queue: nil
                    usingBlock: ^(NSNotification* note)
    {
        exitFakeFullScreen ((NSWindow*) [note object], true);
    }];

    [centre addObserverForName: NSApplicationDidChangeScreenParametersNotification
                        object: nil
                         queue: nil
                    usingBlock: ^(NSNotification*) { refitFakeFullScreenWindows(); }];
}

bool isFakeFullScreenMac (void* peerView)
{
    if (peerView == nullptr)
        return false;

    NSWindow* window = [(__bridge NSView*) peerView window];
    return window != nil && fakeFullScreenWindows().count (keyFor (window)) != 0;
}
#endif
