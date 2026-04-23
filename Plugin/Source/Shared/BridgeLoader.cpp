#include "BridgeLoader.h"

#if JUCE_WINDOWS
 #include <windows.h>
#elif JUCE_MAC || JUCE_LINUX
 #include <dlfcn.h>
#endif

namespace wfs::plugin
{
    BridgeLoader& BridgeLoader::getInstance()
    {
        static BridgeLoader instance;
        return instance;
    }

    BridgeLoader::~BridgeLoader() = default;

    // Find the path of the DLL/dylib that contains this function. Unlike
    // juce::File::getSpecialLocation(currentApplicationFile), which returns
    // the hosting application's executable on Windows, this returns the path
    // of the plugin module that carries our code — exactly what we need for
    // walking up to locate the bridge library sitting next to the VST3.
    static juce::File getPathOfOwnModule()
    {
       #if JUCE_WINDOWS
        HMODULE hModule = nullptr;
        if (GetModuleHandleExA (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
                                  | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                reinterpret_cast<LPCSTR> (&getPathOfOwnModule),
                                &hModule) != 0 && hModule != nullptr)
        {
            char buffer[MAX_PATH * 2] = { 0 };
            const DWORD len = GetModuleFileNameA (hModule, buffer, sizeof (buffer));
            if (len > 0 && len < sizeof (buffer))
                return juce::File (juce::String::fromUTF8 (buffer));
        }
       #elif JUCE_MAC || JUCE_LINUX
        Dl_info info {};
        if (dladdr (reinterpret_cast<const void*> (&getPathOfOwnModule), &info) != 0
            && info.dli_fname != nullptr)
            return juce::File (juce::String::fromUTF8 (info.dli_fname));
       #endif
        return {};
    }

    juce::File BridgeLoader::locateBridgeLibrary() const
    {
       #if JUCE_WINDOWS
        const juce::String libName = "WFS-DIY-PluginBridge.dll";
       #elif JUCE_MAC
        const juce::String libName = "libWFS-DIY-PluginBridge.dylib";
       #else
        const juce::String libName = "libWFS-DIY-PluginBridge.so";
       #endif

        auto ownModule = getPathOfOwnModule();
        if (ownModule != juce::File())
        {
            auto dir = ownModule.isDirectory() ? ownModule : ownModule.getParentDirectory();
            for (int hops = 0; hops < 5; ++hops)
            {
                auto candidate = dir.getChildFile (libName);
                if (candidate.existsAsFile())
                    return candidate;
                dir = dir.getParentDirectory();
            }
        }

       #if JUCE_WINDOWS
        for (const auto* path : { "C:/Program Files/Common Files/VST3/WFS-DIY",
                                   "C:/Program Files/Common Files/VST3" })
        {
            auto candidate = juce::File (path).getChildFile (libName);
            if (candidate.existsAsFile())
                return candidate;
        }
       #elif JUCE_MAC
        for (const auto* path : { "/Library/Audio/Plug-Ins/VST3/WFS-DIY",
                                   "/Library/Audio/Plug-Ins/VST3" })
        {
            auto candidate = juce::File (path).getChildFile (libName);
            if (candidate.existsAsFile())
                return candidate;
        }
       #else
        auto candidate = juce::File ("/usr/local/lib/wfs-diy").getChildFile (libName);
        if (candidate.existsAsFile())
            return candidate;
       #endif
        return {};
    }

    bool BridgeLoader::resolveSymbols()
    {
        if (dll == nullptr)
            return false;

       #define WFS_RESOLVE(name, field) \
            field = reinterpret_cast<decltype(field)> (dll->getFunction (name)); \
            if (field == nullptr) return false;

       // Optional resolves — missing symbols leave the pointer null but
       // don't fail the whole load. Lets older bridge DLLs keep working
       // for plugins that don't use the newer features (e.g. ADM 3f).
       #define WFS_RESOLVE_OPTIONAL(name, field) \
            field = reinterpret_cast<decltype(field)> (dll->getFunction (name));

        WFS_RESOLVE ("wfs_bridge_abi_version",                abiVersion)
        WFS_RESOLVE ("wfs_bridge_master_register",            masterRegister)
        WFS_RESOLVE ("wfs_bridge_master_unregister",          masterUnregister)
        WFS_RESOLVE ("wfs_bridge_master_dispatch_inbound",    masterDispatch)
        WFS_RESOLVE ("wfs_bridge_master_snapshot_input_ids",  masterSnapshotIds)
        WFS_RESOLVE ("wfs_bridge_track_register",             trackRegister)
        WFS_RESOLVE ("wfs_bridge_track_unregister",           trackUnregister)
        WFS_RESOLVE ("wfs_bridge_track_send_outbound",        trackSendOutbound)
        WFS_RESOLVE ("wfs_bridge_track_count",                trackCount)
        WFS_RESOLVE ("wfs_bridge_has_master",                 hasMaster)
        WFS_RESOLVE_OPTIONAL ("wfs_bridge_master_set_outbound_3f",     masterSetOutbound3f)
        WFS_RESOLVE_OPTIONAL ("wfs_bridge_master_dispatch_inbound_3f", masterDispatch3f)
        WFS_RESOLVE_OPTIONAL ("wfs_bridge_track_set_inbound_3f",       trackSetInbound3f)
        WFS_RESOLVE_OPTIONAL ("wfs_bridge_track_send_outbound_3f",     trackSendOutbound3f)
       #undef WFS_RESOLVE
       #undef WFS_RESOLVE_OPTIONAL

        return abiVersion() == kBridgeAbiVersion;
    }

    bool BridgeLoader::ensureLoaded()
    {
        if (dll != nullptr)
            return true;

        auto path = locateBridgeLibrary();
        if (path == juce::File())
            return false;  // nothing to try — Master/Tracks just won't link up

        dll = std::make_unique<juce::DynamicLibrary>();
        if (! dll->open (path.getFullPathName()))
        {
            dll.reset();
            return false;
        }
        if (! resolveSymbols())
        {
            dll->close();
            dll.reset();
            return false;
        }
        return true;
    }
}
