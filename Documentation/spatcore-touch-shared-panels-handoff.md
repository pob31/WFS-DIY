# Handoff — spatcore PR #30: touch panels shared with Go.dot (and any spatcore app)

**For:** the WFS-DIY session on the Mac that moves the spatcore pin.
**Written:** 2026-10-31, from Go.dot's side, which already moved to it (Go.dot
namespace draft §125, its MT.10–MT.11).
**spatcore:** `3e0c940` (main, merge of PR #30, commit `5071ad1`). WFS-DIY pins
`c139a97` today.

---

## 1. Why

WFS-DIY and Go.dot can now both read the same USB touch panel on one Mac. Each
one already injected only the fingers that land on its own windows, because each
asks macOS which window is frontmost. Three things still collided:

- **The cursor.** Each app moved the system cursor for a finger outside its own
  windows. So a finger on Go.dot was also a synthetic click from WFS-DIY, and
  the other way round.
- **Quitting.** The first app to quit put the panel back in mouse mode while the
  other was still using it.
- **Settings.** Each app kept its own panel settings, so the screens had to be
  identified once per app.

PR #30 adds `controllers/touch/TouchPanelRegistry`, plain C++17 and POSIX. The
apps send each other no messages. They hold `flock` locks in
`~/Library/Application Support/spatcore/touch/`, and the kernel releases a lock
when its process ends, even in a crash:

- `readers/<pid>.lock`: one per running reader.
- `pointer.lock`: the one app that moves the cursor. The next app takes over
  when the holder quits.
- `restore.lock`: only the last app to quit puts the panels back in mouse mode.
- `panels.settings`: one settings file for every app.

`MacHidTouchManager` shares by default:

- A finger on another reader's window is ignored here. That app takes it.
- Only the holder of `pointer.lock` moves the cursor.
- On quit, an app restores mouse mode only if it is the last reader.
- New `reloadSettings()` re-reads the shared settings and notifies listeners
  only when something changed.
- New `drivesPointer()` says whether this app drives the cursor.

## 2. What to change in WFS-DIY

### 2.1 Required: the pin and the new file (the Mac build fails without it)

1. Move the pin: `git -C spatcore fetch && git -C spatcore checkout 3e0c940`,
   then `git add spatcore`.
2. **Add the new source to the project.** WFS-DIY lists spatcore's sources one
   by one, and `MacHidTouchManager.mm` now calls the registry. Without it the
   Mac link fails with undefined `spatcore::controllers::TouchPanelRegistry`
   symbols. Windows and Linux still build without it, because nothing there
   calls it.
   - `WFS-DIY.jucer`, beside the `TouchInjector` entries (about line 398):

     ```xml
     <FILE id="tchRegH" name="TouchPanelRegistry.h" compile="0" resource="0"
           file="spatcore/controllers/touch/TouchPanelRegistry.h"/>
     <FILE id="tchRegCpp" name="TouchPanelRegistry.cpp" compile="1" resource="0"
           file="spatcore/controllers/touch/TouchPanelRegistry.cpp"/>
     ```
   - Then re-save in Projucer, so the Xcode project, `WFS-DIY_App.vcxproj` (+
     `.filters`) and `Builds/LinuxMakefile/Makefile` all get it. Or add it by
     hand next to `TouchInjector.cpp` in each, as commit `185ce7ce` did.
   - The `.cpp` compiles to nothing on Windows and to plain POSIX code on Linux,
     so it is harmless in every project.
3. **Run `tools/apply-macos-juce-patches.sh` again** after the submodule update,
   as usual: the update puts JUCE's file back.

### 2.2 Recommended: one line before offering the identify circles

In `Source/MainComponent.cpp`, in the change listener that offers
`startTouchIdentify()` (about line 1146), re-read the shared settings first. A
screen already identified in Go.dot then counts as identified, and WFS-DIY does
not show its circles for it again:

```cpp
            if (juce::Desktop::getInstance().getDisplays().displays.size() < 2)
                return;

            safe->macTouchManager->reloadSettings();   // a screen placed in another spatcore app counts

            for (auto& p : safe->macTouchManager->getPanels())
```

`reloadSettings()` notifies listeners only when the settings actually changed,
so calling it from inside this listener cannot loop. Go.dot makes the same call
in the same place.

### 2.3 Nothing to do: what changes on its own

- **Settings move to `~/Library/Application Support/spatcore/touch/panels.settings`.**
  WFS-DIY's own `macTouchPanels` entry in `WFS-DIY.settings` is copied there on
  the first launch, if the shared file is still empty. The Touch Screens window
  reads and writes the shared file; a save writes only the panel that changed,
  over the file as it stands, so an edit made in Go.dot at the same moment is
  kept. The old entry in `WFS-DIY.settings` is no longer read.
- **"Cursor moves from touches outside WFS-DIY"** still applies per panel. With
  two apps running, only the holder of `pointer.lock` moves the cursor: the
  first app to drive it, then the next one when it quits.
- **Quitting WFS-DIY while Go.dot still runs** leaves the panels in multitouch
  mode, and the log says *"left in multitouch mode: another application still
  reads the panels"*. Go.dot restores mouse mode when it quits.
- **To keep WFS-DIY reading alone, as before:** construct the reader with
  `MacHidTouchManager ("WFS-DIY", false)`. Not recommended.

## 3. Checking it

- **Build** on the Mac. `WFS_TEST_TOUCH_INJECT=1` should still pass: the
  injector is unchanged, and a single reader behaves as before.
- **spatcore's own test:** `testTouchPanelRegistry`, in `spatcore-tests` from
  `spatcore/tests/standalone`. It covers two readers, a reader killed with
  SIGKILL, and a stale lock file swept. It already passed on macOS 15 (Apple
  clang 17) and Ubuntu 24.04 (GCC 13), with `-Werror`.
- **With the ILITEK panel and Go.dot (main at `078876f2` or later), both open:**
  1. A finger on WFS-DIY's Map moves a marker, and Go.dot does nothing (no
     click).
  2. A finger on Go.dot's desk rides a fader, and WFS-DIY does nothing.
  3. A finger on the Finder moves the cursor once, not twice.
  4. Quit one app: the other still gets fingers, and the panel stays in
     multitouch mode. Quit the second: the panel is a mouse again.
  5. With two displays, identify the screens in one app; the other does not
     offer its circles for them.
  6. Kill one app (Force Quit): the other takes over the cursor at the next
     finger outside both apps' windows.

## 4. Changelog line (suggested)

> Touch screens on macOS are shared with other spatcore applications - Go.dot
> among them - on the same computer: one of them moves the cursor, a finger on
> the other's window is left to it, the last to quit puts the panel back in
> mouse mode, and the screens are placed once for all of them (spatcore PR #30).
