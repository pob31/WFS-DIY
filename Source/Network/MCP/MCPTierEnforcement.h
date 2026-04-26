#pragma once

#include <JuceHeader.h>
#include <map>
#include <atomic>

namespace WFSNetwork
{

/** Phase 6 — tier enforcement and operator-side safety controls.

    Tier 1 tools execute immediately. Tier 2 tools require a two-step call
    (first call returns a confirmation token; second call must pass that
    token along with `confirm: true`). Tier 3 adds a per-operator safety
    gate that the operator opens via the UI for ~60 s; AI clients cannot
    open the gate themselves.

    Dry-run mode is a global flag the operator toggles in the UI. When
    enabled, every Tier 1 call is escalated to Tier 2 — useful for
    rehearsal sessions where the operator wants to see exactly what the
    AI would do without it actually happening.

    Threading: this class is owned by MCPServer. All mutating operations
    take their own internal lock; the dispatcher calls into it from the
    JUCE message thread and the UI calls into it from the same thread.
    The 60 s gate auto-close runs on a juce::Timer that ticks at 4 Hz so
    the UI countdown stays smooth without flooding the message queue. */
class MCPTierEnforcement : private juce::Timer
{
public:
    MCPTierEnforcement();
    ~MCPTierEnforcement() override;

    //==========================================================================
    // Decisions returned to the dispatcher.
    //==========================================================================
    enum class Decision
    {
        Execute,            // Tool runs normally
        AwaitConfirmation,  // Returned token must come back with confirm:true
        SafetyGateClosed,   // Tier 3 refused — operator must open the gate
        DryRunAcknowledge   // Dry-run mode: respond as if executed but skip the side effects
    };

    struct Outcome
    {
        Decision decision = Decision::Execute;
        juce::String confirmationToken; // populated when AwaitConfirmation
        int effectiveTier = 1;          // tier after dry-run escalation
        juce::String message;           // human-readable explanation for the AI
        int secondsUntilExpiry = 0;     // for AwaitConfirmation
    };

    /** Evaluate an inbound tool call. Caller passes the tool's declared
        tier and the args; if the args contain a `confirm` token that
        matches a still-valid pending entry for the same tool name, the
        call is approved. Otherwise:
          - Tier 1 (after dry-run escalation): Execute (or DryRunAcknowledge)
          - Tier 2: AwaitConfirmation
          - Tier 3 with closed gate: SafetyGateClosed
          - Tier 3 with open gate: AwaitConfirmation */
    Outcome evaluate (const juce::String& toolName, int declaredTier, const juce::var& args);

    /** Operator-side controls (typically called from the UI). */
    void openSafetyGate();    // opens for kSafetyGateLifetimeSec
    void closeSafetyGate();   // immediate close
    bool isSafetyGateOpen() const noexcept;
    int  secondsUntilGateCloses() const noexcept;  // 0 when closed

    void setDryRunMode (bool on);
    bool isDryRunMode() const noexcept;

    /** Listener for UI to repaint when state changes. */
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void tierEnforcementStateChanged() = 0;
    };
    void addListener    (Listener* l);
    void removeListener (Listener* l);

    static constexpr int kConfirmationLifetimeSec = 30;
    static constexpr int kSafetyGateLifetimeSec   = 60;

private:
    void timerCallback() override;
    void notifyListeners();
    juce::String issueToken (const juce::String& toolName, const juce::var& args);
    bool consumeMatchingToken (const juce::String& toolName, const juce::var& args);
    void purgeExpired();

    struct PendingConfirmation
    {
        juce::String toolName;
        juce::String argsJson;   // canonical JSON so a token only validates the same call
        juce::Time   expiresAt;
    };

    juce::CriticalSection lock;
    std::map<juce::String, PendingConfirmation> pending;  // token -> entry

    std::atomic<bool> dryRun { false };
    juce::Time gateOpenedUntil; // zero-initialised → gate starts closed
    bool gateOpen = false;

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPTierEnforcement)
};

} // namespace WFSNetwork
