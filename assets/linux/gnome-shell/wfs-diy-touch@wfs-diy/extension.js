// GNOME Shell's touchscreen gestures (three-finger swipes for workspaces and
// the overview, the top-edge drag that leaves fullscreen, the bottom-edge drag
// for the on-screen keyboard) are gesture actions on the stage, in the capture
// phase: they see every touch first, and once one recognises a gesture the
// fingers are cancelled for the application underneath. A multitouch WFS-DIY
// then loses touches mid-move. No Wayland or X11 protocol lets an application
// opt out, so this extension turns the stage gestures off while a WFS-DIY
// window is open, and gives each its own state back when the last one closes.

import Clutter from 'gi://Clutter';
import {Extension} from 'resource:///org/gnome/shell/extensions/extension.js';

// WM_CLASS of the windows that hold the gestures off.
const WM_CLASSES = ['WFS-DIY'];

function isGesture(action) {
    return (Clutter.GestureAction !== undefined && action instanceof Clutter.GestureAction)
        || (Clutter.Gesture !== undefined && action instanceof Clutter.Gesture);
}

export default class WfsDiyTouchExtension extends Extension {
    enable() {
        this._windows = new Map(); // Meta.Window -> signal ids
        this._held = new Map();    // gesture action -> {id, wanted}
        this._createdId = global.display.connect('window-created',
            (_display, window) => this._track(window));
        for (const actor of global.get_window_actors())
            this._track(actor.meta_window);
    }

    disable() {
        global.display.disconnect(this._createdId);
        for (const [window, ids] of this._windows)
            ids.forEach(id => window.disconnect(id));
        this._windows.clear();
        this._release();
    }

    _track(window) {
        if (!window || this._windows.has(window))
            return;
        this._windows.set(window, [
            window.connect('notify::wm-class', () => this._update()),
            window.connect('unmanaged', () => {
                this._windows.get(window)?.forEach(id => window.disconnect(id));
                this._windows.delete(window);
                this._update();
            }),
        ]);
        this._update();
    }

    _update() {
        const open = [...this._windows.keys()].some(w =>
            WM_CLASSES.includes(w.get_wm_class()) || WM_CLASSES.includes(w.get_wm_class_instance()));
        if (open)
            this._hold();
        else
            this._release();
    }

    _hold() {
        for (const action of global.stage.get_actions()) {
            if (!isGesture(action) || this._held.has(action))
                continue;
            // The shell re-enables a gesture whenever its swipe tracker is
            // re-enabled (bind_property); remember that wish and keep it off.
            const entry = {wanted: action.enabled, id: 0};
            entry.id = action.connect('notify::enabled', () => {
                if (action.enabled) {
                    entry.wanted = true;
                    action.enabled = false;
                }
            });
            action.enabled = false;
            this._held.set(action, entry);
        }
    }

    _release() {
        for (const [action, entry] of this._held) {
            action.disconnect(entry.id);
            action.enabled = entry.wanted;
        }
        this._held.clear();
    }
}
