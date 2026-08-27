#include "MCPOSCQueryAuditor.h"

namespace WFSNetwork
{

MCPOSCQueryAuditor::MCPOSCQueryAuditor (MCPLogger& logger,
                                        WFSValueTreeState& state,
                                        juce::File generatedJson,
                                        juce::String url)
    : juce::Thread ("MCPOSCQueryAuditor"),
      mcpLogger (logger),
      valueTreeState (state),
      generatedToolsJson (std::move (generatedJson)),
      oscQueryUrl (std::move (url))
{
}

void MCPOSCQueryAuditor::addChannelStrippedForms (std::set<juce::String>& paths)
{
    // OSCQuery publishes /wfs/input/1/attenuation; the manifest declares
    // /wfs/input/attenuation, because the channel is an ARGUMENT there rather
    // than part of the address. Index the live set both ways so the comparison
    // is between like and like - without this every per-channel tool reads as
    // drift, and the real drift drowns.
    std::set<juce::String> extra;
    for (const auto& path : paths)
    {
        auto parts = juce::StringArray::fromTokens (path, "/", "");
        parts.removeEmptyStrings();
        if (parts.size() < 3)
            continue;

        // Drop a purely numeric segment anywhere in the path.
        juce::StringArray kept;
        bool dropped = false;
        for (const auto& part : parts)
        {
            if (! dropped && part.containsOnly ("0123456789") && part.isNotEmpty())
                { dropped = true; continue; }
            kept.add (part);
        }
        if (dropped && ! kept.isEmpty())
            extra.insert ("/" + kept.joinIntoString ("/"));
    }
    paths.insert (extra.begin(), extra.end());
}

MCPOSCQueryAuditor::~MCPOSCQueryAuditor()
{
    stopThread (3000);
}

void MCPOSCQueryAuditor::runAudit()
{
    if (isThreadRunning())
        return;
    startThread (juce::Thread::Priority::low);
}

void MCPOSCQueryAuditor::collectPaths (const juce::var& node,
                                       const juce::String& prefix,
                                       std::set<juce::String>& out)
{
    auto* obj = node.getDynamicObject();
    if (obj == nullptr)
        return;

    // OSCQuery nodes carry FULL_PATH on every node — prefer it over
    // reconstructing from the recursion prefix.
    auto fullPath = obj->getProperty ("FULL_PATH").toString();
    if (fullPath.isNotEmpty())
        out.insert (fullPath);
    else if (prefix.isNotEmpty())
        out.insert (prefix);

    auto contents = obj->getProperty ("CONTENTS");
    if (auto* contentsObj = contents.getDynamicObject())
    {
        const auto& props = contentsObj->getProperties();
        for (int i = 0; i < props.size(); ++i)
        {
            const auto childName = props.getName (i).toString();
            const auto child = props.getValueAt (i);
            const auto childPath = (prefix.isEmpty() || prefix == "/")
                                     ? "/" + childName
                                     : prefix + "/" + childName;
            collectPaths (child, childPath, out);
        }
    }
}

juce::String MCPOSCQueryAuditor::stripPlaceholder (const juce::String& templatePath)
{
    // Templates look like "/wfs/input/arrayAtten{array}" — drop the
    // `{...}` and any trailing characters so we end up with the stem
    // path the family is built around.
    const int braceIdx = templatePath.indexOfChar ('{');
    if (braceIdx < 0)
        return templatePath;
    return templatePath.substring (0, braceIdx);
}

void MCPOSCQueryAuditor::run()
{
    if (threadShouldExit())
        return;

    // Let OSCQuery finish binding its HTTP listener.
    wait (kStartupDelayMs);
    if (threadShouldExit())
        return;

    // 1. Fetch the OSCQuery tree.
    juce::URL url (oscQueryUrl);
    auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs (kHttpTimeoutMs);

    auto stream = url.createInputStream (options);
    if (stream == nullptr)
    {
        mcpLogger.logInfo ("OSCQuery audit skipped: server not reachable at " + oscQueryUrl);
        return;
    }

    const auto responseText = stream->readEntireStreamAsString();
    if (threadShouldExit() || responseText.isEmpty())
    {
        mcpLogger.logInfo ("OSCQuery audit skipped: empty response from " + oscQueryUrl);
        return;
    }

    const auto tree = juce::JSON::parse (responseText);
    if (! tree.isObject())
    {
        mcpLogger.logError ("OSCQuery audit: response is not a JSON object");
        return;
    }

    std::set<juce::String> liveTreePaths;
    collectPaths (tree, "/", liveTreePaths);
    addChannelStrippedForms (liveTreePaths);

    if (liveTreePaths.empty())
    {
        mcpLogger.logError ("OSCQuery audit: live tree has no paths - server returned an empty namespace");
        return;
    }

    // 2. Walk generated_tools.json.
    if (! generatedToolsJson.existsAsFile())
    {
        mcpLogger.logError ("OSCQuery audit skipped: generated_tools.json not found at "
                            + generatedToolsJson.getFullPathName());
        return;
    }

    const auto generated = juce::JSON::parse (generatedToolsJson);
    auto* generatedObj = generated.getDynamicObject();
    if (generatedObj == nullptr)
    {
        mcpLogger.logError ("OSCQuery audit: generated_tools.json is not a JSON object");
        return;
    }

    int totalChecked = 0;
    int totalMissing = 0;
    int loggedMissing = 0;

    auto checkArray = [&] (const juce::var& arr)
    {
        if (! arr.isArray()) return;
        for (const auto& entry : *arr.getArray())
        {
            auto* entryObj = entry.getDynamicObject();
            if (entryObj == nullptr) continue;

            juce::String path;
            if (entryObj->hasProperty ("internal_osc_path"))
                path = entryObj->getProperty ("internal_osc_path").toString();
            else if (entryObj->hasProperty ("internal_osc_path_template"))
                path = stripPlaceholder (entryObj->getProperty ("internal_osc_path_template").toString());

            if (path.isEmpty())
                continue;

            ++totalChecked;
            if (liveTreePaths.find (path) != liveTreePaths.end())
                continue;

            ++totalMissing;
            if (loggedMissing < kMaxDriftLogsPerRun)
            {
                const auto toolName = entryObj->getProperty ("name").toString();
                mcpLogger.logInfo ("OSCQuery: tool '" + toolName
                                    + "' declares " + path
                                    + " but no OSCQuery node exists");
                ++loggedMissing;
            }
        }
    };

    checkArray (generatedObj->getProperty ("tools"));
    checkArray (generatedObj->getProperty ("nudge_tools"));

    // Informational, not an error, and that is a correction rather than a
    // softening. `internal_osc_path` is a convention string the generator
    // synthesises; it is never validated against OSCMessageRouter, and whole
    // namespaces (/wfs/cluster, /wfs/network) have no OSCQuery subtree at all.
    // More to the point, an MCP call sends no OSC packet - it writes the
    // ValueTree directly - so a missing path has never been the reason an MCP
    // tool failed. What it does tell you is that the parameter has no OSC or
    // OSCQuery route, which matters to an OSC client and not to this one. The
    // writability check below is the one that finds broken tools.
    mcpLogger.logInfo ("OSCQuery audit: " + juce::String (totalChecked)
                        + " tool paths checked, " + juce::String (totalMissing)
                        + " with no OSCQuery node (informational: these are "
                          "MCP-reachable but have no OSC route; showed first "
                        + juce::String (loggedMissing) + ")");

    // Second, independent check. Path presence says nothing about whether the
    // app can STORE the parameter: a node can be published and the write still
    // dropped, which is exactly how a quarter of this surface went inert. Shared
    // with the WFS_TEST_MCP_SURFACE self-test so the two cannot disagree.
    const auto surface = SurfaceAudit::run (generated, valueTreeState);

    int loggedDead = 0;
    for (const auto& line : surface.deadDetails)
    {
        if (loggedDead >= kMaxDriftLogsPerRun)
            break;
        mcpLogger.logError ("MCP surface: " + line);
        ++loggedDead;
    }

    if (surface.deadCount() == 0)
        mcpLogger.logInfo ("MCP surface: " + SurfaceAudit::summarise (surface));
    else
        mcpLogger.logError ("MCP surface: " + SurfaceAudit::summarise (surface)
                             + " (showed first " + juce::String (loggedDead) + ")");
}

} // namespace WFSNetwork
