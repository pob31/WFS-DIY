#pragma once

#include <juce_core/juce_core.h>

namespace WFSNetwork
{

/** Dispatcher-facing slice of the app's undo engine.

    The full undo engine stays app-side for now — it applies before/after
    payloads to the app's ValueTree (the opaque write-target seam is
    deferred, see docs/architecture/core-boundary-proposal-control.md
    §2.6) — but the core dispatcher only needs these two hooks. */
class MCPUndoHooks
{
public:
    virtual ~MCPUndoHooks() = default;

    /** Called whenever a new state-modifying tool call lands. Standard
        undo/redo semantics: a fresh action invalidates any pending redo
        history. */
    virtual void onNewStateModifyingRecord() = 0;

    /** Return + clear queued cross-actor notifications. The dispatcher
        calls this when assembling each tools/call response and, if
        non-empty, attaches the array to the result envelope as
        `notifications`. */
    virtual juce::Array<juce::var> drainPendingNotifications() = 0;
};

} // namespace WFSNetwork
