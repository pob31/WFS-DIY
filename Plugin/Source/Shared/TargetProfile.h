#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <juce_core/juce_core.h>
#include <map>
#include <mutex>
#include <vector>
#include "PluginAdmMapping.h"

namespace wfs::plugin
{
    enum class ProfileFlow      { Bidirectional, SendOnly };
    enum class CoordTarget      { WfsCartMeters, AdmNormalizedXyz, AdmAed, CustomPerAxis };

    /** A user-facing target processor configuration.
        Decides UI fields, connection flow, address translation, and (for ADM)
        the normalisation/scale used when crossing between metres and [-1, 1]. */
    struct TargetProfile
    {
        juce::String id;
        juce::String displayName;
        ProfileFlow  flow              { ProfileFlow::Bidirectional };
        bool         showHttpField     { true };
        bool         showAdmRxField    { true };
        bool         admEchoEnabled    { false };  // SendOnly profiles can opt in to receiving ADM echoes

        CoordTarget  coordTarget       { CoordTarget::WfsCartMeters };

        PluginAdmMapping::CartesianMappingConfig admCart;
        PluginAdmMapping::PolarMappingConfig     admPolar;

        // For Custom profile: param name -> address template (with {id} placeholder).
        // Reserved keys: "positionX", "positionY", "positionZ" (per-axis 1f),
        //                "positionXYZ" (combined 3f), "positionAed" (combined 3f).
        std::map<juce::String, juce::String> paramAddressMap;
    };

    /** Holds three built-in profiles plus tracks the active one.
        Persists into a small ValueTree node alongside the APVTS state. */
    class TargetProfileRegistry
    {
    public:
        static constexpr const char* kIdWfsDiy  = "wfs-diy";
        static constexpr const char* kIdAdmOsc  = "adm-osc";
        static constexpr const char* kIdCustom  = "custom";

        TargetProfileRegistry();

        const TargetProfile& get (const juce::String& id) const;
        TargetProfile&       get (const juce::String& id);

        const TargetProfile& active() const  { return get (activeId); }
        TargetProfile&       active()        { return get (activeId); }

        juce::String activeId { kIdWfsDiy };
        juce::StringArray allIds() const;

        juce::ValueTree toState() const;
        void            fromState (const juce::ValueTree&);

    private:
        std::map<juce::String, TargetProfile> profiles;
    };

    /** One outbound OSC event ready for the transport. */
    struct OutEvent
    {
        bool         isThreeFloat { false };
        juce::String path;
        int          channelId    { 0 };  // unused when isThreeFloat
        float        v1           { 0.0f };
        float        v2           { 0.0f };
        float        v3           { 0.0f };
    };

    /** Converts incoming (Track-side) OSC events to outgoing events shaped for
        the active profile. Maintains a small per-channel position cache so
        per-axis updates can be re-projected as combined ADM triples, and so
        that switching profiles mid-session preserves the last known position. */
    class TargetProfileTranslator
    {
    public:
        explicit TargetProfileTranslator (TargetProfileRegistry& r) : registry (r) {}

        void setVariantTag   (int channelId, const juce::String& tag);
        void clearVariantTag (int channelId);
        void clearAll();

        /** Single-float events from the bridge (typically /wfs/input/<param>). */
        void translate1f (const juce::String& path, int channelId, float value,
                          std::vector<OutEvent>& out);

        /** Three-float events from the bridge (typically /adm/obj/<id>/xyz|aed). */
        void translate3f (const juce::String& path, float v1, float v2, float v3,
                          std::vector<OutEvent>& out);

    private:
        struct CachedPos { float x = 0.0f; float y = 0.0f; float z = 0.0f; bool valid = false; };

        void updateCacheAxis (int channelId, int axis, float value);
        void updateCacheXYZ  (int channelId, float x, float y, float z);
        CachedPos getCache   (int channelId);

        void emitPositionForActive (int channelId, std::vector<OutEvent>& out);
        void emitNonPosition       (const juce::String& paramName, int channelId, float value,
                                    std::vector<OutEvent>& out);

        static juce::String substituteId (const juce::String& tmpl, int channelId);

        TargetProfileRegistry& registry;
        std::mutex             stateLock;
        std::map<int, CachedPos>     positionCache;
        std::map<int, juce::String>  variantTags;
    };
}
