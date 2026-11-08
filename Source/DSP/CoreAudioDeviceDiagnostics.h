#pragma once

#include <JuceHeader.h>
#include "../WFSLogger.h"

#if JUCE_MAC
 #include <CoreAudio/CoreAudio.h>
 #include <CoreFoundation/CoreFoundation.h>
 #include <unistd.h>
 #include <atomic>
 #include <vector>
#endif

/**
    CoreAudioDeviceDiagnostics

    Logs what the CoreAudio HAL knows about the device JUCE opened, and keeps
    logging when it changes: clock source, nominal and measured sample rate,
    buffer frame size, alive / running state, exclusive (hog) ownership, and
    processor overloads as the HAL reports them.

    JUCE hides all of this behind AudioIODevice. A show rig with periodic clicks
    and no xrun in our callback (the 2026-11-06 Digiface Dante session) left
    nothing in the log to tell a Dante clock change from a USB hiccup from our
    own DSP; this is the missing instrumentation. Read-only: nothing here calls
    a setter, and JUCE's own listeners keep restarting the device as before.

    On every other platform the class is an empty shell so the call sites need
    no #if.

    Threading: attach() / detach() on the message thread. The HAL delivers
    property notifications on its own (non-realtime) thread; the listener only
    reads properties and writes to WFSLogger, which locks. detach() must run
    before the logger goes away - MainComponent's destructor does that first.
*/
class CoreAudioDeviceDiagnostics
{
public:
    CoreAudioDeviceDiagnostics() = default;
    ~CoreAudioDeviceDiagnostics() { detach(); }

    /** Resolves the device JUCE names, logs a one-line snapshot and starts
        listening. Calling it again for the same name is a no-op, so it can sit
        in the device-changed callback. */
    void attach (const juce::String& juceDeviceName);

    /** Stops listening. Safe to call twice or without attach(). */
    void detach();

private:
   #if JUCE_MAC
    // Equal to kAudioObjectPropertyElementMain (12.0+) and ...Master (older):
    // both are 0, and spelling the constant avoids the deprecation warning on
    // either SDK.
    static constexpr AudioObjectPropertyElement kElement = 0;

    static OSStatus listenerProc (AudioObjectID, UInt32, const AudioObjectPropertyAddress*, void*);
    void propertyChanged (const AudioObjectPropertyAddress& addr);

    static bool getData (AudioObjectID id, AudioObjectPropertySelector sel,
                         AudioObjectPropertyScope scope, void* out, UInt32 size);
    static bool hasProperty (AudioObjectID id, AudioObjectPropertySelector sel,
                             AudioObjectPropertyScope scope);
    static juce::String getCFString (AudioObjectID id, AudioObjectPropertySelector sel,
                                     AudioObjectPropertyScope scope);
    static juce::String getUInt32Text (AudioObjectID id, AudioObjectPropertySelector sel,
                                       AudioObjectPropertyScope scope);
    static AudioObjectID findDevice (const juce::String& juceDeviceName);
    static juce::String transportTypeName (UInt32 t);

    juce::String clockSourceText() const;
    juce::String hogText() const;
    juce::String snapshot() const;

    AudioObjectID deviceId = kAudioObjectUnknown;
    juce::String attachedName;
    std::vector<AudioObjectPropertyAddress> listened;

    // Overloads arrive one notification each and can come in bursts: counted
    // on the HAL thread, written once per second at most.
    std::atomic<uint32_t> overloadsPending { 0 };
    std::atomic<uint32_t> overloadsLastLogMs { 0 };
    uint32_t overloadsTotal = 0;   // HAL thread only
   #endif
};

//==============================================================================
#if JUCE_MAC

inline bool CoreAudioDeviceDiagnostics::getData (AudioObjectID id, AudioObjectPropertySelector sel,
                                                 AudioObjectPropertyScope scope, void* out, UInt32 size)
{
    const AudioObjectPropertyAddress addr { sel, scope, kElement };
    UInt32 sz = size;
    return AudioObjectGetPropertyData (id, &addr, 0, nullptr, &sz, out) == noErr && sz == size;
}

inline bool CoreAudioDeviceDiagnostics::hasProperty (AudioObjectID id, AudioObjectPropertySelector sel,
                                                     AudioObjectPropertyScope scope)
{
    const AudioObjectPropertyAddress addr { sel, scope, kElement };
    return AudioObjectHasProperty (id, &addr);
}

inline juce::String CoreAudioDeviceDiagnostics::getCFString (AudioObjectID id, AudioObjectPropertySelector sel,
                                                             AudioObjectPropertyScope scope)
{
    CFStringRef s = nullptr;
    if (! getData (id, sel, scope, &s, sizeof (s)) || s == nullptr)
        return {};
    const juce::String result = juce::String::fromCFString (s);
    CFRelease (s);
    return result;
}

inline juce::String CoreAudioDeviceDiagnostics::getUInt32Text (AudioObjectID id, AudioObjectPropertySelector sel,
                                                               AudioObjectPropertyScope scope)
{
    UInt32 v = 0;
    return getData (id, sel, scope, &v, sizeof (v)) ? juce::String ((int) v) : juce::String ("?");
}

inline juce::String CoreAudioDeviceDiagnostics::transportTypeName (UInt32 t)
{
    switch (t)
    {
        case kAudioDeviceTransportTypeBuiltIn:     return "built-in";
        case kAudioDeviceTransportTypeAggregate:   return "aggregate";
        case kAudioDeviceTransportTypeVirtual:     return "virtual";
        case kAudioDeviceTransportTypePCI:         return "PCI";
        case kAudioDeviceTransportTypeUSB:         return "USB";
        case kAudioDeviceTransportTypeFireWire:    return "FireWire";
        case kAudioDeviceTransportTypeBluetooth:   return "Bluetooth";
        case kAudioDeviceTransportTypeHDMI:        return "HDMI";
        case kAudioDeviceTransportTypeDisplayPort: return "DisplayPort";
        case kAudioDeviceTransportTypeAirPlay:     return "AirPlay";
        case kAudioDeviceTransportTypeAVB:         return "AVB";
        case kAudioDeviceTransportTypeThunderbolt: return "Thunderbolt";
        case kAudioDeviceTransportTypeUnknown:     return "unknown";
        default:                                   return "0x" + juce::String::toHexString ((int) t);
    }
}

inline AudioObjectID CoreAudioDeviceDiagnostics::findDevice (const juce::String& juceDeviceName)
{
    const AudioObjectPropertyAddress addr { kAudioHardwarePropertyDevices,
                                            kAudioObjectPropertyScopeGlobal, kElement };
    UInt32 size = 0;
    if (AudioObjectGetPropertyDataSize (kAudioObjectSystemObject, &addr, 0, nullptr, &size) != noErr || size == 0)
        return kAudioObjectUnknown;

    std::vector<AudioObjectID> ids (size / sizeof (AudioObjectID));
    if (AudioObjectGetPropertyData (kAudioObjectSystemObject, &addr, 0, nullptr, &size, ids.data()) != noErr)
        return kAudioObjectUnknown;

    // JUCE names a plain device after kAudioObjectPropertyName. When its input
    // and output come from different devices it builds an aggregate and names
    // it after both; the first device whose name is contained in JUCE's is
    // then the best available answer (the output side, which is what clocks
    // the rig).
    AudioObjectID partial = kAudioObjectUnknown;
    for (const auto id : ids)
    {
        const auto name = getCFString (id, kAudioObjectPropertyName, kAudioObjectPropertyScopeGlobal);
        if (name.isEmpty())
            continue;
        if (name == juceDeviceName)
            return id;
        if (partial == kAudioObjectUnknown && juceDeviceName.contains (name))
            partial = id;
    }
    return partial;
}

inline juce::String CoreAudioDeviceDiagnostics::clockSourceText() const
{
    // Most devices have no selectable clock source and no property at all;
    // Dante, MADI and ADAT interfaces are exactly the ones that do.
    if (! hasProperty (deviceId, kAudioDevicePropertyClockSource, kAudioObjectPropertyScopeGlobal))
        return "n/a";

    UInt32 current = 0;
    if (! getData (deviceId, kAudioDevicePropertyClockSource, kAudioObjectPropertyScopeGlobal, &current, sizeof (current)))
        return "?";

    auto nameOf = [this] (UInt32 sourceId) -> juce::String
    {
        CFStringRef s = nullptr;
        AudioValueTranslation tr { &sourceId, sizeof (sourceId), &s, sizeof (s) };
        const AudioObjectPropertyAddress addr { kAudioDevicePropertyClockSourceNameForIDCFString,
                                                kAudioObjectPropertyScopeGlobal, kElement };
        UInt32 sz = sizeof (tr);
        if (AudioObjectGetPropertyData (deviceId, &addr, 0, nullptr, &sz, &tr) != noErr || s == nullptr)
            return "#" + juce::String ((int) sourceId);
        const juce::String name = juce::String::fromCFString (s);
        CFRelease (s);
        return name;
    };

    juce::String text = "'" + nameOf (current) + "'";

    const AudioObjectPropertyAddress listAddr { kAudioDevicePropertyClockSources,
                                                kAudioObjectPropertyScopeGlobal, kElement };
    UInt32 size = 0;
    if (AudioObjectGetPropertyDataSize (deviceId, &listAddr, 0, nullptr, &size) == noErr && size > 0)
    {
        std::vector<UInt32> sources (size / sizeof (UInt32));
        if (AudioObjectGetPropertyData (deviceId, &listAddr, 0, nullptr, &size, sources.data()) == noErr)
        {
            juce::StringArray names;
            for (const auto s : sources)
                names.add (nameOf (s));
            text += " of [" + names.joinIntoString (", ") + "]";
        }
    }
    return text;
}

inline juce::String CoreAudioDeviceDiagnostics::hogText() const
{
    pid_t pid = -1;
    if (! getData (deviceId, kAudioDevicePropertyHogMode, kAudioObjectPropertyScopeGlobal, &pid, sizeof (pid)))
        return "n/a";
    if (pid == -1)
        return "none";
    return "pid " + juce::String ((int) pid) + (pid == getpid() ? " (this app)" : " (ANOTHER app)");
}

inline juce::String CoreAudioDeviceDiagnostics::snapshot() const
{
    Float64 nominal = 0, actual = 0;
    getData (deviceId, kAudioDevicePropertyNominalSampleRate, kAudioObjectPropertyScopeGlobal, &nominal, sizeof (nominal));
    getData (deviceId, kAudioDevicePropertyActualSampleRate,  kAudioObjectPropertyScopeGlobal, &actual,  sizeof (actual));

    UInt32 transport = 0, running = 0, alive = 0;
    getData (deviceId, kAudioDevicePropertyTransportType,   kAudioObjectPropertyScopeGlobal, &transport, sizeof (transport));
    getData (deviceId, kAudioDevicePropertyDeviceIsRunning, kAudioObjectPropertyScopeGlobal, &running,   sizeof (running));
    getData (deviceId, kAudioDevicePropertyDeviceIsAlive,   kAudioObjectPropertyScopeGlobal, &alive,     sizeof (alive));

    juce::String s;
    s << "CoreAudio '" << attachedName << "' (id " << (int) deviceId
      << ", uid " << getCFString (deviceId, kAudioDevicePropertyDeviceUID, kAudioObjectPropertyScopeGlobal)
      << ", " << transportTypeName (transport) << "): "
      << "nominal " << juce::String (nominal, 0) << " Hz, measured " << juce::String (actual, 3) << " Hz"
      << ", buffer " << getUInt32Text (deviceId, kAudioDevicePropertyBufferFrameSize, kAudioObjectPropertyScopeGlobal)
      << ", clock source " << clockSourceText()
      << ", latency in " << getUInt32Text (deviceId, kAudioDevicePropertyLatency, kAudioObjectPropertyScopeInput)
      << " / out " << getUInt32Text (deviceId, kAudioDevicePropertyLatency, kAudioObjectPropertyScopeOutput)
      << " (safety " << getUInt32Text (deviceId, kAudioDevicePropertySafetyOffset, kAudioObjectPropertyScopeInput)
      << " / " << getUInt32Text (deviceId, kAudioDevicePropertySafetyOffset, kAudioObjectPropertyScopeOutput) << ")"
      << ", alive " << (alive != 0 ? "yes" : "NO")
      << ", running " << (running != 0 ? "yes" : "no")
      << ", hog " << hogText();
    return s;
}

inline void CoreAudioDeviceDiagnostics::attach (const juce::String& juceDeviceName)
{
    if (juceDeviceName == attachedName && deviceId != kAudioObjectUnknown)
    {
        // Same name, same object: nothing to do. Same name but the object is
        // gone (unplugged and replugged, the HAL hands out a new id) means the
        // listeners sit on a corpse, so fall through and resolve again.
        UInt32 alive = 0;
        if (getData (deviceId, kAudioDevicePropertyDeviceIsAlive, kAudioObjectPropertyScopeGlobal, &alive, sizeof (alive))
            && alive != 0)
            return;
        WFSLogger::getInstance().logInfo ("CoreAudio: '" + juceDeviceName + "' is a new HAL object - re-attaching");
    }

    detach();
    attachedName = juceDeviceName;
    deviceId = findDevice (juceDeviceName);

    if (deviceId == kAudioObjectUnknown)
    {
        WFSLogger::getInstance().logInfo ("CoreAudio: no HAL device matches '" + juceDeviceName
                                          + "' - device diagnostics unavailable");
        return;
    }

    WFSLogger::getInstance().logInfo (snapshot());

    const AudioObjectPropertySelector selectors[] =
    {
        kAudioDevicePropertyClockSource,
        kAudioDevicePropertyNominalSampleRate,
        kAudioDevicePropertyBufferFrameSize,
        kAudioDevicePropertyDeviceIsAlive,
        kAudioDevicePropertyDeviceIsRunning,
        kAudioDevicePropertyHogMode,
        kAudioDevicePropertyDeviceHasChanged,
        kAudioDeviceProcessorOverload,
    };

    for (const auto sel : selectors)
    {
        const AudioObjectPropertyAddress addr { sel, kAudioObjectPropertyScopeGlobal, kElement };
        if (AudioObjectAddPropertyListener (deviceId, &addr, &CoreAudioDeviceDiagnostics::listenerProc, this) == noErr)
            listened.push_back (addr);
    }
}

inline void CoreAudioDeviceDiagnostics::detach()
{
    if (deviceId != kAudioObjectUnknown)
        for (const auto& addr : listened)
            AudioObjectRemovePropertyListener (deviceId, &addr, &CoreAudioDeviceDiagnostics::listenerProc, this);

    listened.clear();
    deviceId = kAudioObjectUnknown;
    attachedName.clear();
}

inline OSStatus CoreAudioDeviceDiagnostics::listenerProc (AudioObjectID, UInt32 count,
                                                          const AudioObjectPropertyAddress* addrs, void* client)
{
    auto* self = static_cast<CoreAudioDeviceDiagnostics*> (client);
    for (UInt32 i = 0; i < count; ++i)
        self->propertyChanged (addrs[i]);
    return noErr;
}

inline void CoreAudioDeviceDiagnostics::propertyChanged (const AudioObjectPropertyAddress& addr)
{
    auto& log = WFSLogger::getInstance();
    const juce::String prefix = "CoreAudio '" + attachedName + "': ";

    switch (addr.mSelector)
    {
        case kAudioDevicePropertyClockSource:
            log.logWarning (prefix + "clock source now " + clockSourceText());
            break;

        case kAudioDevicePropertyNominalSampleRate:
        {
            Float64 sr = 0;
            getData (deviceId, kAudioDevicePropertyNominalSampleRate, kAudioObjectPropertyScopeGlobal, &sr, sizeof (sr));
            log.logWarning (prefix + "nominal sample rate now " + juce::String (sr, 0) + " Hz");
            break;
        }

        case kAudioDevicePropertyBufferFrameSize:
            log.logInfo (prefix + "buffer frame size now "
                         + getUInt32Text (deviceId, kAudioDevicePropertyBufferFrameSize, kAudioObjectPropertyScopeGlobal));
            break;

        case kAudioDevicePropertyDeviceIsAlive:
        {
            UInt32 alive = 0;
            getData (deviceId, kAudioDevicePropertyDeviceIsAlive, kAudioObjectPropertyScopeGlobal, &alive, sizeof (alive));
            if (alive != 0) log.logInfo (prefix + "device is alive again");
            else            log.logWarning (prefix + "device is GONE (not alive)");
            break;
        }

        case kAudioDevicePropertyDeviceIsRunning:
        {
            UInt32 running = 0;
            getData (deviceId, kAudioDevicePropertyDeviceIsRunning, kAudioObjectPropertyScopeGlobal, &running, sizeof (running));
            log.logInfo (prefix + (running != 0 ? "running" : "stopped"));
            break;
        }

        case kAudioDevicePropertyHogMode:
            log.logWarning (prefix + "exclusive access now " + hogText());
            break;

        case kAudioDevicePropertyDeviceHasChanged:
            log.logInfo (prefix + "configuration changed - " + snapshot());
            break;

        case kAudioDeviceProcessorOverload:
        {
            // One notification per late IO cycle. Our callback already logs the
            // counter it polls; this is the HAL's own word, timestamped where
            // the overload happened, and rate-limited so a storm stays one line
            // per second.
            const uint32_t pending = overloadsPending.fetch_add (1, std::memory_order_relaxed) + 1;
            const uint32_t now = juce::Time::getMillisecondCounter();
            if (now - overloadsLastLogMs.load (std::memory_order_relaxed) >= 1000)
            {
                overloadsLastLogMs.store (now, std::memory_order_relaxed);
                overloadsPending.store (0, std::memory_order_relaxed);
                overloadsTotal += pending;
                log.logWarning (prefix + "HAL processor overload x" + juce::String ((int) pending)
                                + " (total " + juce::String ((int) overloadsTotal) + ")");
            }
            break;
        }

        default:
            break;
    }
}

#else // ! JUCE_MAC

inline void CoreAudioDeviceDiagnostics::attach (const juce::String&) {}
inline void CoreAudioDeviceDiagnostics::detach() {}

#endif
