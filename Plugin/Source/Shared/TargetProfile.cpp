#include "TargetProfile.h"

namespace wfs::plugin
{
    namespace
    {
        const juce::Identifier idTargetProfileState ("TargetProfileState");
        const juce::Identifier idActiveProfile      ("activeProfile");
        const juce::Identifier idProfile            ("Profile");
        const juce::Identifier idProfileId          ("id");
        const juce::Identifier idDisplayName        ("displayName");
        const juce::Identifier idAdmEchoEnabled     ("admEchoEnabled");
        const juce::Identifier idCoordTarget        ("coordTarget");
        const juce::Identifier idAdmCart            ("AdmCart");
        const juce::Identifier idAdmPolar           ("AdmPolar");
        const juce::Identifier idAxis               ("Axis");
        const juce::Identifier idAxisIndex          ("axisIndex");
        const juce::Identifier idAxisSwap           ("axisSwap");
        const juce::Identifier idSignFlip           ("signFlip");
        const juce::Identifier idCenterOffset       ("centerOffset");
        const juce::Identifier idBreakpoint         ("breakpoint");
        const juce::Identifier idPosInnerWidth      ("posInnerWidth");
        const juce::Identifier idPosOuterWidth      ("posOuterWidth");
        const juce::Identifier idNegInnerWidth      ("negInnerWidth");
        const juce::Identifier idNegOuterWidth      ("negOuterWidth");
        const juce::Identifier idAzimuthOffset      ("azimuthOffset");
        const juce::Identifier idAzimuthFlip        ("azimuthFlip");
        const juce::Identifier idElevationFlip      ("elevationFlip");
        const juce::Identifier idDistBreakpoint     ("distBreakpoint");
        const juce::Identifier idDistInner          ("distInner");
        const juce::Identifier idDistOuter          ("distOuter");
        const juce::Identifier idDistCenter         ("distCenter");
        const juce::Identifier idAddressMap         ("AddressMap");
        const juce::Identifier idMapEntry           ("Entry");
        const juce::Identifier idMapKey             ("key");
        const juce::Identifier idMapValue           ("value");

        TargetProfile makeWfsDiy()
        {
            TargetProfile p;
            p.id             = TargetProfileRegistry::kIdWfsDiy;
            p.displayName    = "WFS-DIY";
            p.flow           = ProfileFlow::Bidirectional;
            p.showHttpField  = true;
            p.showAdmRxField = true;
            p.coordTarget    = CoordTarget::WfsCartMeters;
            return p;
        }

        TargetProfile makeAdmOsc()
        {
            TargetProfile p;
            p.id             = TargetProfileRegistry::kIdAdmOsc;
            p.displayName    = "ADM-OSC";
            p.flow           = ProfileFlow::SendOnly;
            p.showHttpField  = false;
            p.showAdmRxField = true;   // shown but only used when admEchoEnabled
            p.coordTarget    = CoordTarget::AdmNormalizedXyz;
            // Defaults: 5m inner / 5m outer per axis, breakpoint 0.5
            return p;
        }

        TargetProfile makeCustom()
        {
            TargetProfile p;
            p.id             = TargetProfileRegistry::kIdCustom;
            p.displayName    = "Custom";
            p.flow           = ProfileFlow::SendOnly;
            p.showHttpField  = false;
            p.showAdmRxField = false;
            p.coordTarget    = CoordTarget::WfsCartMeters;
            // Empty address map by default.
            return p;
        }

        juce::String coordTargetToString (CoordTarget t)
        {
            switch (t)
            {
                case CoordTarget::WfsCartMeters:    return "wfsCartMeters";
                case CoordTarget::AdmNormalizedXyz: return "admNormalizedXyz";
                case CoordTarget::AdmAed:           return "admAed";
                case CoordTarget::CustomPerAxis:    return "customPerAxis";
            }
            return "wfsCartMeters";
        }

        CoordTarget coordTargetFromString (const juce::String& s)
        {
            if (s == "admNormalizedXyz") return CoordTarget::AdmNormalizedXyz;
            if (s == "admAed")           return CoordTarget::AdmAed;
            if (s == "customPerAxis")    return CoordTarget::CustomPerAxis;
            return CoordTarget::WfsCartMeters;
        }

        void cartCfgToTree (const PluginAdmMapping::CartesianMappingConfig& cfg, juce::ValueTree& parent)
        {
            juce::ValueTree node (idAdmCart);
            for (int a = 0; a < 3; ++a)
            {
                juce::ValueTree axis (idAxis);
                axis.setProperty (idAxisIndex,    a, nullptr);
                axis.setProperty (idAxisSwap,     cfg.axes[a].axisSwap, nullptr);
                axis.setProperty (idSignFlip,     cfg.axes[a].signFlip ? 1 : 0, nullptr);
                axis.setProperty (idCenterOffset, cfg.axes[a].centerOffset, nullptr);
                axis.setProperty (idBreakpoint,   cfg.axes[a].breakpoint, nullptr);
                axis.setProperty (idPosInnerWidth, cfg.axes[a].posInnerWidth, nullptr);
                axis.setProperty (idPosOuterWidth, cfg.axes[a].posOuterWidth, nullptr);
                axis.setProperty (idNegInnerWidth, cfg.axes[a].negInnerWidth, nullptr);
                axis.setProperty (idNegOuterWidth, cfg.axes[a].negOuterWidth, nullptr);
                node.appendChild (axis, nullptr);
            }
            parent.appendChild (node, nullptr);
        }

        void cartCfgFromTree (PluginAdmMapping::CartesianMappingConfig& cfg, const juce::ValueTree& parent)
        {
            const auto node = parent.getChildWithName (idAdmCart);
            if (! node.isValid()) return;
            for (int c = 0; c < node.getNumChildren(); ++c)
            {
                const auto axis = node.getChild (c);
                if (axis.getType() != idAxis) continue;
                const int a = static_cast<int> (axis.getProperty (idAxisIndex, 0));
                if (a < 0 || a > 2) continue;
                cfg.axes[a].axisSwap      = static_cast<int>   (axis.getProperty (idAxisSwap, a));
                cfg.axes[a].signFlip      = static_cast<int>   (axis.getProperty (idSignFlip, 0)) != 0;
                cfg.axes[a].centerOffset  = static_cast<float> (axis.getProperty (idCenterOffset, 0.0f));
                cfg.axes[a].breakpoint    = static_cast<float> (axis.getProperty (idBreakpoint, 0.5f));
                cfg.axes[a].posInnerWidth = static_cast<float> (axis.getProperty (idPosInnerWidth, 5.0f));
                cfg.axes[a].posOuterWidth = static_cast<float> (axis.getProperty (idPosOuterWidth, 5.0f));
                cfg.axes[a].negInnerWidth = static_cast<float> (axis.getProperty (idNegInnerWidth, 5.0f));
                cfg.axes[a].negOuterWidth = static_cast<float> (axis.getProperty (idNegOuterWidth, 5.0f));
            }
        }

        void polarCfgToTree (const PluginAdmMapping::PolarMappingConfig& cfg, juce::ValueTree& parent)
        {
            juce::ValueTree node (idAdmPolar);
            node.setProperty (idAzimuthOffset,  cfg.azimuthOffset, nullptr);
            node.setProperty (idAzimuthFlip,    cfg.azimuthFlip ? 1 : 0, nullptr);
            node.setProperty (idElevationFlip,  cfg.elevationFlip ? 1 : 0, nullptr);
            node.setProperty (idDistBreakpoint, cfg.distBreakpoint, nullptr);
            node.setProperty (idDistInner,      cfg.distInner, nullptr);
            node.setProperty (idDistOuter,      cfg.distOuter, nullptr);
            node.setProperty (idDistCenter,     cfg.distCenter, nullptr);
            parent.appendChild (node, nullptr);
        }

        void polarCfgFromTree (PluginAdmMapping::PolarMappingConfig& cfg, const juce::ValueTree& parent)
        {
            const auto node = parent.getChildWithName (idAdmPolar);
            if (! node.isValid()) return;
            cfg.azimuthOffset  = static_cast<float> (node.getProperty (idAzimuthOffset, 0.0f));
            cfg.azimuthFlip    = static_cast<int>   (node.getProperty (idAzimuthFlip, 0)) != 0;
            cfg.elevationFlip  = static_cast<int>   (node.getProperty (idElevationFlip, 0)) != 0;
            cfg.distBreakpoint = static_cast<float> (node.getProperty (idDistBreakpoint, 0.5f));
            cfg.distInner      = static_cast<float> (node.getProperty (idDistInner, 5.0f));
            cfg.distOuter      = static_cast<float> (node.getProperty (idDistOuter, 5.0f));
            cfg.distCenter     = static_cast<float> (node.getProperty (idDistCenter, 0.0f));
        }

        void mapToTree (const std::map<juce::String, juce::String>& m, juce::ValueTree& parent)
        {
            juce::ValueTree node (idAddressMap);
            for (const auto& [k, v] : m)
            {
                juce::ValueTree entry (idMapEntry);
                entry.setProperty (idMapKey,   k, nullptr);
                entry.setProperty (idMapValue, v, nullptr);
                node.appendChild (entry, nullptr);
            }
            parent.appendChild (node, nullptr);
        }

        void mapFromTree (std::map<juce::String, juce::String>& m, const juce::ValueTree& parent)
        {
            const auto node = parent.getChildWithName (idAddressMap);
            if (! node.isValid()) return;
            m.clear();
            for (int c = 0; c < node.getNumChildren(); ++c)
            {
                const auto entry = node.getChild (c);
                if (entry.getType() != idMapEntry) continue;
                const juce::String k = entry.getProperty (idMapKey).toString();
                const juce::String v = entry.getProperty (idMapValue).toString();
                if (k.isNotEmpty()) m[k] = v;
            }
        }

        juce::ValueTree profileToTree (const TargetProfile& p)
        {
            juce::ValueTree node (idProfile);
            node.setProperty (idProfileId,      p.id, nullptr);
            node.setProperty (idDisplayName,    p.displayName, nullptr);
            node.setProperty (idAdmEchoEnabled, p.admEchoEnabled ? 1 : 0, nullptr);
            node.setProperty (idCoordTarget,    coordTargetToString (p.coordTarget), nullptr);
            cartCfgToTree (p.admCart, node);
            polarCfgToTree (p.admPolar, node);
            mapToTree (p.paramAddressMap, node);
            return node;
        }

        void profileFromTree (TargetProfile& p, const juce::ValueTree& node)
        {
            p.admEchoEnabled = static_cast<int> (node.getProperty (idAdmEchoEnabled, p.admEchoEnabled ? 1 : 0)) != 0;
            const juce::String ct = node.getProperty (idCoordTarget, coordTargetToString (p.coordTarget)).toString();
            p.coordTarget = coordTargetFromString (ct);
            cartCfgFromTree (p.admCart, node);
            polarCfgFromTree (p.admPolar, node);
            mapFromTree (p.paramAddressMap, node);
        }
    }

    //==========================================================================
    // TargetProfileRegistry
    //==========================================================================

    TargetProfileRegistry::TargetProfileRegistry()
    {
        profiles[kIdWfsDiy] = makeWfsDiy();
        profiles[kIdAdmOsc] = makeAdmOsc();
        profiles[kIdCustom] = makeCustom();
    }

    const TargetProfile& TargetProfileRegistry::get (const juce::String& id) const
    {
        auto it = profiles.find (id);
        if (it != profiles.end())
            return it->second;
        return profiles.find (kIdWfsDiy)->second;
    }

    TargetProfile& TargetProfileRegistry::get (const juce::String& id)
    {
        auto it = profiles.find (id);
        if (it != profiles.end())
            return it->second;
        return profiles.find (kIdWfsDiy)->second;
    }

    juce::StringArray TargetProfileRegistry::allIds() const
    {
        return { kIdWfsDiy, kIdAdmOsc, kIdCustom };
    }

    juce::ValueTree TargetProfileRegistry::toState() const
    {
        juce::ValueTree root (idTargetProfileState);
        root.setProperty (idActiveProfile, activeId, nullptr);
        for (const auto& id : allIds())
        {
            const auto it = profiles.find (id);
            if (it != profiles.end())
                root.appendChild (profileToTree (it->second), nullptr);
        }
        return root;
    }

    void TargetProfileRegistry::fromState (const juce::ValueTree& root)
    {
        if (! root.isValid() || root.getType() != idTargetProfileState)
            return;
        const juce::String storedActive = root.getProperty (idActiveProfile).toString();
        if (storedActive.isNotEmpty())
            activeId = storedActive;
        for (int c = 0; c < root.getNumChildren(); ++c)
        {
            const auto child = root.getChild (c);
            if (child.getType() != idProfile) continue;
            const juce::String id = child.getProperty (idProfileId).toString();
            auto it = profiles.find (id);
            if (it == profiles.end()) continue;
            profileFromTree (it->second, child);
        }
    }

    //==========================================================================
    // TargetProfileTranslator
    //==========================================================================

    void TargetProfileTranslator::setVariantTag (int channelId, const juce::String& tag)
    {
        std::lock_guard<std::mutex> sl (stateLock);
        variantTags[channelId] = tag;
    }

    void TargetProfileTranslator::clearVariantTag (int channelId)
    {
        std::lock_guard<std::mutex> sl (stateLock);
        variantTags.erase (channelId);
        positionCache.erase (channelId);
    }

    void TargetProfileTranslator::clearAll()
    {
        std::lock_guard<std::mutex> sl (stateLock);
        variantTags.clear();
        positionCache.clear();
    }

    void TargetProfileTranslator::updateCacheAxis (int channelId, int axis, float value)
    {
        std::lock_guard<std::mutex> sl (stateLock);
        auto& c = positionCache[channelId];
        if (axis == 0) c.x = value;
        else if (axis == 1) c.y = value;
        else if (axis == 2) c.z = value;
        c.valid = true;
    }

    void TargetProfileTranslator::updateCacheXYZ (int channelId, float x, float y, float z)
    {
        std::lock_guard<std::mutex> sl (stateLock);
        auto& c = positionCache[channelId];
        c.x = x; c.y = y; c.z = z; c.valid = true;
    }

    TargetProfileTranslator::CachedPos TargetProfileTranslator::getCache (int channelId)
    {
        std::lock_guard<std::mutex> sl (stateLock);
        auto it = positionCache.find (channelId);
        if (it == positionCache.end()) return {};
        return it->second;
    }

    juce::String TargetProfileTranslator::substituteId (const juce::String& tmpl, int channelId)
    {
        return tmpl.replace ("{id}", juce::String (channelId))
                   .replace ("{ch}", juce::String (channelId));
    }

    void TargetProfileTranslator::emitPositionForActive (int channelId, std::vector<OutEvent>& out)
    {
        const auto& profile = registry.active();
        const auto pos = getCache (channelId);
        if (! pos.valid) return;

        switch (profile.coordTarget)
        {
            case CoordTarget::WfsCartMeters:
            {
                // Per-axis 1f. WFS-DIY profile path: emit the standard /wfs/input/positionX|Y|Z.
                // Custom profile path: look up address templates if present, else fall back to standard.
                const bool isCustom = (profile.id == TargetProfileRegistry::kIdCustom);
                auto pickPath = [&] (const juce::String& reservedKey, const juce::String& fallback) -> juce::String
                {
                    if (isCustom)
                    {
                        auto it = profile.paramAddressMap.find (reservedKey);
                        if (it != profile.paramAddressMap.end() && it->second.isNotEmpty())
                            return substituteId (it->second, channelId);
                    }
                    return fallback;
                };
                out.push_back ({ false, pickPath ("positionX", "/wfs/input/positionX"), channelId, pos.x, 0, 0 });
                out.push_back ({ false, pickPath ("positionY", "/wfs/input/positionY"), channelId, pos.y, 0, 0 });
                out.push_back ({ false, pickPath ("positionZ", "/wfs/input/positionZ"), channelId, pos.z, 0, 0 });
                return;
            }

            case CoordTarget::AdmNormalizedXyz:
            {
                float ax = 0.0f, ay = 0.0f, az = 0.0f;
                PluginAdmMapping::inverseCartesianMapping (pos.x, pos.y, pos.z, profile.admCart, ax, ay, az);
                juce::String path = "/adm/obj/" + juce::String (channelId) + "/xyz";
                if (profile.id == TargetProfileRegistry::kIdCustom)
                {
                    auto it = profile.paramAddressMap.find ("positionXYZ");
                    if (it != profile.paramAddressMap.end() && it->second.isNotEmpty())
                        path = substituteId (it->second, channelId);
                }
                out.push_back ({ true, path, 0, ax, ay, az });
                return;
            }

            case CoordTarget::AdmAed:
            {
                float aaz = 0.0f, ael = 0.0f, ad = 0.0f;
                PluginAdmMapping::inversePolarMapping (pos.x, pos.y, pos.z, profile.admPolar, aaz, ael, ad);
                juce::String path = "/adm/obj/" + juce::String (channelId) + "/aed";
                if (profile.id == TargetProfileRegistry::kIdCustom)
                {
                    auto it = profile.paramAddressMap.find ("positionAed");
                    if (it != profile.paramAddressMap.end() && it->second.isNotEmpty())
                        path = substituteId (it->second, channelId);
                }
                out.push_back ({ true, path, 0, aaz, ael, ad });
                return;
            }

            case CoordTarget::CustomPerAxis:
            {
                auto emit = [&] (const juce::String& key, float v)
                {
                    auto it = profile.paramAddressMap.find (key);
                    if (it == profile.paramAddressMap.end() || it->second.isEmpty()) return;
                    out.push_back ({ false, substituteId (it->second, channelId), channelId, v, 0, 0 });
                };
                emit ("positionX", pos.x);
                emit ("positionY", pos.y);
                emit ("positionZ", pos.z);
                return;
            }
        }
    }

    void TargetProfileTranslator::emitNonPosition (const juce::String& paramName, int channelId,
                                                    float value, std::vector<OutEvent>& out)
    {
        const auto& profile = registry.active();

        if (profile.id == TargetProfileRegistry::kIdWfsDiy)
        {
            // Passthrough using the standard /wfs/input/<param> shape.
            out.push_back ({ false, "/wfs/input/" + paramName, channelId, value, 0, 0 });
            return;
        }

        // ADM-OSC and Custom: only emit if the profile declares an address.
        auto it = profile.paramAddressMap.find (paramName);
        if (it == profile.paramAddressMap.end() || it->second.isEmpty())
            return;
        out.push_back ({ false, substituteId (it->second, channelId), channelId, value, 0, 0 });
    }

    void TargetProfileTranslator::translate1f (const juce::String& path, int channelId, float value,
                                                std::vector<OutEvent>& out)
    {
        if (! path.startsWith ("/wfs/input/"))
        {
            // Unknown family; pass through only on WFS-DIY.
            if (registry.activeId == TargetProfileRegistry::kIdWfsDiy)
                out.push_back ({ false, path, channelId, value, 0, 0 });
            return;
        }

        const juce::String paramName = path.fromFirstOccurrenceOf ("/wfs/input/", false, false);

        if (paramName == "positionX" || paramName == "positionY" || paramName == "positionZ")
        {
            const int axis = (paramName == "positionX") ? 0 : (paramName == "positionY" ? 1 : 2);
            updateCacheAxis (channelId, axis, value);
            emitPositionForActive (channelId, out);
            return;
        }

        emitNonPosition (paramName, channelId, value, out);
    }

    void TargetProfileTranslator::translate3f (const juce::String& path, float v1, float v2, float v3,
                                                std::vector<OutEvent>& out)
    {
        // Expected: /adm/obj/<id>/xyz or /aed
        if (! path.startsWith ("/adm/obj/"))
        {
            if (registry.activeId == TargetProfileRegistry::kIdWfsDiy)
                out.push_back ({ true, path, 0, v1, v2, v3 });
            return;
        }

        const juce::String tail = path.fromFirstOccurrenceOf ("/adm/obj/", false, false);
        const int slashIdx = tail.indexOf ("/");
        if (slashIdx <= 0)
            return;
        const juce::String idStr = tail.substring (0, slashIdx);
        if (! idStr.containsOnly ("0123456789"))
            return;
        const int channelId = idStr.getIntValue();
        const juce::String suffix = tail.substring (slashIdx + 1);

        // Always update canonical cache by converting through the active profile's
        // mapping. (For ADM source = ADM target with same config, this is a no-op.)
        const auto& profile = registry.active();
        if (suffix == "xyz")
        {
            const auto cart = PluginAdmMapping::applyCartesianMapping (v1, v2, v3, profile.admCart);
            updateCacheXYZ (channelId, cart.x, cart.y, cart.z);
        }
        else if (suffix == "aed")
        {
            const auto cart = PluginAdmMapping::applyPolarMapping (v1, v2, v3, profile.admPolar);
            updateCacheXYZ (channelId, cart.x, cart.y, cart.z);
        }
        else
        {
            // Unknown ADM tail (e.g. /x, /y, /z per-axis) — pass through on WFS-DIY only.
            if (registry.activeId == TargetProfileRegistry::kIdWfsDiy)
                out.push_back ({ true, path, 0, v1, v2, v3 });
            return;
        }

        emitPositionForActive (channelId, out);
    }
}
