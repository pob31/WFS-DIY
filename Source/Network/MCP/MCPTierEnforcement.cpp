#include "MCPTierEnforcement.h"

namespace WFSNetwork
{

namespace
{
    juce::String canonicalArgsJson (const juce::var& args)
    {
        // Strip the `confirm` field before canonicalising so the token
        // matches the original (un-confirmed) call exactly.
        if (! args.isObject())
            return juce::JSON::toString (args, true);
        auto* obj = args.getDynamicObject();
        if (obj == nullptr)
            return juce::JSON::toString (args, true);

        auto cloned = std::make_unique<juce::DynamicObject>();
        for (int i = 0; i < obj->getProperties().size(); ++i)
        {
            const auto name = obj->getProperties().getName (i);
            if (name.toString() == "confirm")
                continue;
            cloned->setProperty (name, obj->getProperties().getValueAt (i));
        }
        return juce::JSON::toString (juce::var (cloned.release()), true);
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

        gateStillOpen = gateOpen;
    }

    // Notify whenever a state transition happened OR the gate is still
    // open — the latter keeps the UI's depleting-fill countdown ticking
    // smoothly. 4 Hz repaint of a single button is cheap; the listener
    // list is empty by default and currently has at most one entry
    // (NetworkTab), so the cost is negligible.
    if (changed || gateStillOpen)
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

    // Tier 3: enforce the safety gate first. Operator must have allowed
    // critical actions (UI button, MIDI / StreamDeck binding once those
    // land); AI cannot allow them itself.
    if (out.effectiveTier == 3 && ! isSafetyGateOpen())
    {
        out.decision = Decision::SafetyGateClosed;
        out.message  = "Critical AI actions are blocked. Ask the operator "
                       "to allow critical actions (Network tab) before "
                       "retrying. They auto-block again "
                       + juce::String (kSafetyGateLifetimeSec)
                       + " seconds after the operator allows them.";
        return out;
    }

    // Tier 2 (and Tier 3 with open gate): two-step confirm.
    if (consumeMatchingToken (toolName, args))
    {
        out.decision = Decision::Execute;
        return out;
    }

    // Issue a fresh token and ask the AI to retry with confirm: <token>.
    out.confirmationToken    = issueToken (toolName, args);
    out.secondsUntilExpiry   = kConfirmationLifetimeSec;
    out.decision             = Decision::AwaitConfirmation;
    if (out.effectiveTier == 3)
    {
        out.message = "Tier 3 action awaiting confirmation. Re-call with "
                      "`confirm: \"" + out.confirmationToken
                      + "\"` within " + juce::String (kConfirmationLifetimeSec)
                      + " seconds. Critical AI actions are currently allowed.";
    }
    else
    {
        out.message = "Tier 2 action awaiting confirmation. Re-call with "
                      "`confirm: \"" + out.confirmationToken
                      + "\"` within " + juce::String (kConfirmationLifetimeSec)
                      + " seconds.";
    }
    return out;
}

void MCPTierEnforcement::openSafetyGate()
{
    {
        const juce::ScopedLock sl (lock);
        gateOpen          = true;
        gateOpenedUntil   = juce::Time::getCurrentTime()
                              + juce::RelativeTime::seconds (kSafetyGateLifetimeSec);
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
