#include "MCPTierEnforcement.h"
#include <algorithm>
#include <vector>

namespace WFSNetwork
{

namespace
{
    /** Recursive type-canonical serializer for tier-2 confirmation tokens.

        Goals:
          - `1` and `1.0` produce the same canonical string (numeric type
            collapsed to a normalized double form).
          - Object keys are sorted lexicographically so an AI may reorder
            keys between the issue-token and confirm-token calls without
            failing the token match.
          - Arrays preserve order (ordered semantically by the AI).
          - Top-level `confirm` field is stripped so the canonical form
            of the first call (no token) matches the second call (with
            token) on every other arg. */
    juce::String canonicalize (const juce::var& v);

    juce::String canonicalizeNumber (double d)
    {
        if (std::isnan (d))      return "\"NaN\"";
        if (std::isinf (d))      return d > 0 ? "\"+Inf\"" : "\"-Inf\"";
        // Fixed-precision form, then strip trailing zeros and a dangling
        // decimal point so that 1, 1.0, 1.00 all collapse to "1".
        juce::String s (d, 9);
        if (s.contains ("."))
        {
            while (s.endsWith ("0")) s = s.dropLastCharacters (1);
            if (s.endsWith (".")) s = s.dropLastCharacters (1);
        }
        return s;
    }

    juce::String canonicalizeObject (const juce::DynamicObject& obj, bool stripConfirm)
    {
        std::vector<juce::String> keys;
        keys.reserve ((size_t) obj.getProperties().size());
        for (int i = 0; i < obj.getProperties().size(); ++i)
        {
            const auto name = obj.getProperties().getName (i).toString();
            if (stripConfirm && name == "confirm")
                continue;
            keys.push_back (name);
        }
        std::sort (keys.begin(), keys.end(),
                   [] (const juce::String& a, const juce::String& b) { return a < b; });

        juce::String out = "{";
        bool first = true;
        for (const auto& k : keys)
        {
            if (! first) out += ",";
            first = false;
            out += juce::JSON::toString (juce::var (k), true);  // quoted, escaped
            out += ":";
            out += canonicalize (obj.getProperty (juce::Identifier (k)));
        }
        out += "}";
        return out;
    }

    juce::String canonicalize (const juce::var& v)
    {
        if (v.isVoid() || v.isUndefined())  return "null";
        if (v.isBool())                     return static_cast<bool> (v) ? "true" : "false";
        if (v.isInt() || v.isInt64() || v.isDouble())
            return canonicalizeNumber (static_cast<double> (v));
        if (v.isString())
            return juce::JSON::toString (v, true);  // quoted + escaped
        if (v.isArray())
        {
            juce::String out = "[";
            const auto* arr = v.getArray();
            for (int i = 0; i < arr->size(); ++i)
            {
                if (i > 0) out += ",";
                out += canonicalize ((*arr)[i]);
            }
            out += "]";
            return out;
        }
        if (auto* obj = v.getDynamicObject())
            return canonicalizeObject (*obj, /* stripConfirm */ false);
        // Fallback for anything else (binary blobs etc.) — let JUCE handle it.
        return juce::JSON::toString (v, true);
    }

    juce::String canonicalArgsJson (const juce::var& args)
    {
        // Top-level: strip `confirm` so the un-confirmed call (issuing
        // the token) and the confirmed call (presenting the token) hash
        // to the same canonical form on every other arg.
        if (auto* obj = args.getDynamicObject())
            return canonicalizeObject (*obj, /* stripConfirm */ true);
        return canonicalize (args);
    }

    juce::String generateToken()
    {
        return juce::Uuid().toDashedString();
    }
}

MCPTierEnforcement::MCPTierEnforcement()
{
    startTimer (250);  // 4 Hz — drives token expiry + gate auto-close + UI countdown
}

MCPTierEnforcement::~MCPTierEnforcement()
{
    stopTimer();
}

void MCPTierEnforcement::timerCallback()
{
    bool changed = false;
    bool gateStillOpen = false;
    bool tier2StillOpen = false;
    {
        const juce::ScopedLock sl (lock);
        const auto now = juce::Time::getCurrentTime();

        // Token expiry
        for (auto it = pending.begin(); it != pending.end(); )
        {
            if (it->second.expiresAt <= now)
            {
                it = pending.erase (it);
                changed = true;
            }
            else
                ++it;
        }

        // Gate auto-close
        if (gateOpen && gateOpenedUntil <= now)
        {
            gateOpen = false;
            changed = true;
        }

        // Tier-2 auto-confirm auto-close
        if (tier2AutoConfirmOpen && tier2AutoConfirmUntil <= now)
        {
            tier2AutoConfirmOpen = false;
            changed = true;
        }

        gateStillOpen  = gateOpen;
        tier2StillOpen = tier2AutoConfirmOpen;
    }

    // State transitions notify immediately. Countdown ticks are throttled
    // to kCountdownNotifyIntervalMs (10 s) — at the 10-minute gate or the
    // 5-minute tier-2 window, 4 Hz UI repaints would advance the depleting
    // fill by less than a pixel per tick. Throttling gives a visible step.
    bool shouldTickCountdown = false;
    if (gateStillOpen || tier2StillOpen)
    {
        const auto nowMs = static_cast<juce::int64> (juce::Time::getMillisecondCounter());
        if (nowMs - lastCountdownNotifyMs >= kCountdownNotifyIntervalMs)
        {
            shouldTickCountdown = true;
            lastCountdownNotifyMs = nowMs;
        }
    }

    if (changed || shouldTickCountdown)
        notifyListeners();
}

void MCPTierEnforcement::notifyListeners()
{
    listeners.call ([] (Listener& l) { l.tierEnforcementStateChanged(); });
}

juce::String MCPTierEnforcement::issueToken (const juce::String& toolName, const juce::var& args)
{
    PendingConfirmation entry;
    entry.toolName  = toolName;
    entry.argsJson  = canonicalArgsJson (args);
    entry.expiresAt = juce::Time::getCurrentTime()
                       + juce::RelativeTime::seconds (kConfirmationLifetimeSec);

    const auto token = generateToken();
    {
        const juce::ScopedLock sl (lock);
        pending[token] = std::move (entry);
    }
    return token;
}

bool MCPTierEnforcement::consumeMatchingToken (const juce::String& toolName, const juce::var& args)
{
    if (! args.isObject())
        return false;
    auto* obj = args.getDynamicObject();
    if (obj == nullptr || ! obj->hasProperty ("confirm"))
        return false;

    const auto presented = obj->getProperty ("confirm").toString();
    if (presented.isEmpty())
        return false;

    const auto canonical = canonicalArgsJson (args);

    const juce::ScopedLock sl (lock);
    auto it = pending.find (presented);
    if (it == pending.end())
        return false;
    if (it->second.toolName != toolName)
        return false;
    if (it->second.argsJson != canonical)
        return false;
    if (it->second.expiresAt <= juce::Time::getCurrentTime())
    {
        pending.erase (it);
        return false;
    }
    pending.erase (it);
    return true;
}

bool MCPTierEnforcement::peekExpiredMatch (const juce::String& toolName,
                                            const juce::var& args) const
{
    if (! args.isObject())
        return false;
    auto* obj = args.getDynamicObject();
    if (obj == nullptr || ! obj->hasProperty ("confirm"))
        return false;
    const auto presented = obj->getProperty ("confirm").toString();
    if (presented.isEmpty())
        return false;

    const auto canonical = canonicalArgsJson (args);

    const juce::ScopedLock sl (lock);
    auto it = pending.find (presented);
    if (it == pending.end())
        return false;
    if (it->second.toolName != toolName)
        return false;
    if (it->second.argsJson != canonical)
        return false;
    return it->second.expiresAt <= juce::Time::getCurrentTime();
}

void MCPTierEnforcement::purgeExpired()
{
    const juce::ScopedLock sl (lock);
    const auto now = juce::Time::getCurrentTime();
    for (auto it = pending.begin(); it != pending.end(); )
    {
        if (it->second.expiresAt <= now) it = pending.erase (it);
        else                              ++it;
    }
}

MCPTierEnforcement::Outcome MCPTierEnforcement::evaluate (const juce::String& toolName,
                                                          int declaredTier,
                                                          const juce::var& args)
{
    // Detect "your previous token rotated" BEFORE purging — purgeExpired
    // erases the entry, after which we can no longer distinguish the
    // expiry case from a never-issued-token case.
    const bool sawExpiredMatch = peekExpiredMatch (toolName, args);
    purgeExpired();

    Outcome out;
    out.effectiveTier = juce::jlimit (1, 3, declaredTier);

    // Master toggle: if the operator has turned AI off, every tool call
    // is refused regardless of tier.
    if (! aiEnabled.load())
    {
        out.decision = Decision::AIDisabled;
        out.message  = "AI is disabled by the operator (Network tab). Ask "
                       "the operator to re-enable it before retrying.";
        return out;
    }

    if (out.effectiveTier == 1)
    {
        out.decision = Decision::Execute;
        return out;
    }

    // Tier 3 needs the safety gate to be open; without it, refused
    // outright (no token offered). The operator opens the gate from
    // the Network tab and it auto-closes after 5 minutes.
    if (out.effectiveTier == 3 && ! isSafetyGateOpen())
    {
        out.decision = Decision::SafetyGateClosed;
        out.message  = "Critical AI actions are blocked. Ask the operator "
                       "to allow critical actions (Network tab, 5-minute "
                       "window) before retrying.";
        return out;
    }

    // Open-gate semantics: the safety gate is a superset of tier-2
    // auto-confirm. When it's open, both tier-2 and tier-3 calls
    // execute immediately (the operator has already consented to the
    // full destructive bracket; making them confirm each one would
    // make the gate redundant). Tier-2 auto-confirm by itself only
    // skips the handshake for tier-2 (tier-3 was already refused
    // above, so this branch is only reached for tier-2/3-with-gate).
    if (isSafetyGateOpen())
    {
        out.decision = Decision::Execute;
        return out;
    }

    // Tier-2 session override: when the operator has consented to a
    // batch of tier-2 work for the next ~5 minutes, skip the per-call
    // handshake. (Reachable only when the safety gate is closed —
    // tier-3 was refused above, so this is tier-2-only.)
    if (out.effectiveTier == 2 && isTier2AutoConfirmActive())
    {
        out.decision = Decision::Execute;
        return out;
    }

    // Tier 2 with no operator-side override: two-step confirm.
    if (consumeMatchingToken (toolName, args))
    {
        out.decision = Decision::Execute;
        return out;
    }

    // Issue a fresh token and ask the AI to retry with confirm: <token>.
    out.confirmationToken    = issueToken (toolName, args);
    out.secondsUntilExpiry   = kConfirmationLifetimeSec;
    out.decision             = Decision::AwaitConfirmation;
    out.tokenExpiredRecovery = sawExpiredMatch;

    juce::String prefix;
    if (sawExpiredMatch)
        prefix = "Previous confirmation token expired (>"
                 + juce::String (kConfirmationLifetimeSec)
                 + "s round-trip). New token issued. ";

    // After the open-gate / auto-confirm short-circuits above, this path
    // is only reached for tier-2 calls with both windows closed. Tier-3
    // is either Execute (gate open) or SafetyGateClosed (gate closed).
    out.message = prefix
                  + "Tier 2 action awaiting confirmation. Re-call with "
                  "`confirm: \"" + out.confirmationToken
                  + "\"` within " + juce::String (kConfirmationLifetimeSec)
                  + " seconds.";
    return out;
}

void MCPTierEnforcement::openSafetyGate()
{
    {
        const juce::ScopedLock sl (lock);
        gateOpen          = true;
        gateOpenedUntil   = juce::Time::getCurrentTime()
                              + juce::RelativeTime::seconds (kSafetyGateLifetimeSec);
        // Reset the throttle so the FIRST countdown tick happens
        // ~10 s after open (not on the next 250 ms internal tick).
        lastCountdownNotifyMs = static_cast<juce::int64> (juce::Time::getMillisecondCounter());
    }
    notifyListeners();
}

void MCPTierEnforcement::closeSafetyGate()
{
    {
        const juce::ScopedLock sl (lock);
        gateOpen          = false;
        gateOpenedUntil   = juce::Time();
    }
    notifyListeners();
}

bool MCPTierEnforcement::isSafetyGateOpen() const noexcept
{
    const juce::ScopedLock sl (lock);
    return gateOpen && gateOpenedUntil > juce::Time::getCurrentTime();
}

int MCPTierEnforcement::secondsUntilGateCloses() const noexcept
{
    const juce::ScopedLock sl (lock);
    if (! gateOpen) return 0;
    const auto remaining = (gateOpenedUntil - juce::Time::getCurrentTime()).inSeconds();
    return juce::jmax (0, static_cast<int> (remaining + 0.5));
}

void MCPTierEnforcement::openTier2AutoConfirm()
{
    {
        const juce::ScopedLock sl (lock);
        tier2AutoConfirmOpen  = true;
        tier2AutoConfirmUntil = juce::Time::getCurrentTime()
                                  + juce::RelativeTime::seconds (kTier2AutoConfirmLifetimeSec);
        lastCountdownNotifyMs = static_cast<juce::int64> (juce::Time::getMillisecondCounter());
    }
    notifyListeners();
}

void MCPTierEnforcement::closeTier2AutoConfirm()
{
    {
        const juce::ScopedLock sl (lock);
        tier2AutoConfirmOpen  = false;
        tier2AutoConfirmUntil = juce::Time();
    }
    notifyListeners();
}

bool MCPTierEnforcement::isTier2AutoConfirmActive() const noexcept
{
    const juce::ScopedLock sl (lock);
    return tier2AutoConfirmOpen && tier2AutoConfirmUntil > juce::Time::getCurrentTime();
}

int MCPTierEnforcement::secondsUntilTier2AutoConfirmCloses() const noexcept
{
    const juce::ScopedLock sl (lock);
    if (! tier2AutoConfirmOpen) return 0;
    const auto remaining = (tier2AutoConfirmUntil - juce::Time::getCurrentTime()).inSeconds();
    return juce::jmax (0, static_cast<int> (remaining + 0.5));
}

void MCPTierEnforcement::setAIEnabled (bool on)
{
    if (aiEnabled.exchange (on) != on)
        notifyListeners();
}

bool MCPTierEnforcement::isAIEnabled() const noexcept
{
    return aiEnabled.load();
}

void MCPTierEnforcement::addListener    (Listener* l) { listeners.add    (l); }
void MCPTierEnforcement::removeListener (Listener* l) { listeners.remove (l); }

} // namespace WFSNetwork
