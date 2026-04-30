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

    Master AI toggle: the operator can disable the AI entirely from the
    UI. When disabled, every tool call returns AIDisabled and nothing
    executes — simplest mental model for the operator ("AI does what it
    asks" vs "AI does nothing"). The safety gate still exists for the
    finer-grained "destructive vs not" distinction within the enabled
    state.

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
        SafetyGateClosed,   // Tier 3 refused — operator must allow critical actions
        AIDisabled          // Operator has the master AI toggle off — all tools refused
    };

    struct Outcome
    {
        Decision decision = Decision::Execute;
        juce::String confirmationToken; // populated when AwaitConfirmation
        int effectiveTier = 1;          // tier after dry-run escalation
        juce::String message;           // human-readable explanation for the AI
        int secondsUntilExpiry = 0;     // for AwaitConfirmation

        // Set when the AI sent a `confirm` token whose entry was already
        // expired before this call landed. The dispatcher surfaces this as
        // `token_expired_recovery: true` in the response payload so the
        // operator and the AI can both notice the round-trip-too-slow case
        // (instead of it looking identical to "first call, no token").
        bool tokenExpiredRecovery = false;
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

    /** Tier-2 session override. While active, evaluate() short-circuits
        Tier-2 calls to Decision::Execute without issuing a confirmation
        token — the operator has consented to a batch of Tier-2 work for
        the duration. Tier-3 still requires a token AND the safety gate.
        Defaults to closed; the operator opens it from the Network tab.
        Independent of the safety gate. */
    void openTier2AutoConfirm();
    void closeTier2AutoConfirm();
    bool isTier2AutoConfirmActive() const noexcept;
    int  secondsUntilTier2AutoConfirmCloses() const noexcept;

    /** Master AI toggle. When false, evaluate() returns AIDisabled for
        every tool call regardless of tier. Defaults to true (AI on). */
    void setAIEnabled (bool on);
    bool isAIEnabled() const noexcept;

    /** Listener for UI to repaint when state changes. */
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void tierEnforcementStateChanged() = 0;
    };
    void addListener    (Listener* l);
    void removeListener (Listener* l);

    static constexpr int kConfirmationLifetimeSec       = 30;
    static constexpr int kSafetyGateLifetimeSec         = 600;    // 10 minutes
    static constexpr int kTier2AutoConfirmLifetimeSec   = 300;    // 5 minutes
    static constexpr int kCountdownNotifyIntervalMs     = 10000;  // UI tick cadence while gate is open

private:
    void timerCallback() override;
    void notifyListeners();
    juce::String issueToken (const juce::String& toolName, const juce::var& args);
    bool consumeMatchingToken (const juce::String& toolName, const juce::var& args);
    /** Returns true if the AI presented a `confirm` token whose pending
        entry matches (toolName, args) but is already past its expiry.
        Callers should treat this as the diagnostic "your previous token
        rotated" signal. Does NOT erase or mutate the entry; the next
        purgeExpired() call clears it normally. */
    bool peekExpiredMatch (const juce::String& toolName, const juce::var& args) const;
    void purgeExpired();

    struct PendingConfirmation
    {
        juce::String toolName;
        juce::String argsJson;   // canonical JSON so a token only validates the same call
        juce::Time   expiresAt;
    };

    juce::CriticalSection lock;
    std::map<juce::String, PendingConfirmation> pending;  // token -> entry

    // Defaults to false — operators must explicitly opt the AI in via the
    // Network-tab toggle. Conservative default; the operator decides when
    // an AI session is active.
    std::atomic<bool> aiEnabled { false };
    juce::Time gateOpenedUntil; // zero-initialised → gate starts closed
    bool gateOpen = false;
    juce::Time tier2AutoConfirmUntil;  // zero-initialised → off
    bool tier2AutoConfirmOpen = false;
    // Phase 8: throttle for the countdown UI tick. The 4 Hz internal
    // timer still runs (token expiry needs 250 ms precision) but
    // listener notifications fire at most once every kCountdownNotifyIntervalMs
    // while the gate is open. State transitions still notify immediately.
    juce::int64 lastCountdownNotifyMs = 0;

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPTierEnforcement)
};

} // namespace WFSNetwork
