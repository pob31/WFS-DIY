#include "MCPOSCQueryAuditor.h"

namespace WFSNetwork
{

MCPOSCQueryAuditor::MCPOSCQueryAuditor (MCPLogger& logger,
                                        juce::File generatedJson,
                                        juce::String url)
    : juce::Thread ("MCPOSCQueryAuditor"),
      mcpLogger (logger),
      generatedToolsJson (std::move (generatedJson)),
      oscQueryUrl (std::move (url))
{
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

    if (liveTreePaths.empty())
    {
        mcpLogger.logError ("OSCQuery audit: live tree has no paths — server returned an empty namespace");
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
                mcpLogger.logError ("OSCQuery drift: tool '" + toolName
                                     + "' declares " + path
                                     + " but no OSCQuery node exists");
                ++loggedMissing;
            }
        }
    };

    checkArray (generatedObj->getProperty ("tools"));
    checkArray (generatedObj->getProperty ("nudge_tools"));

    if (totalMissing == 0)
    {
        mcpLogger.logInfo ("OSCQuery audit: " + juce::String (totalChecked)
                            + " tool paths checked, 0 missing");
    }
    else
    {
        mcpLogger.logInfo ("OSCQuery audit: " + juce::String (totalChecked)
                            + " tool paths checked, " + juce::String (totalMissing)
                            + " missing in live tree (showed first "
                            + juce::String (loggedMissing) + ")");
    }
}

} // namespace WFSNetwork
