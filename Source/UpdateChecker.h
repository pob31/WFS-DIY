#pragma once

#include <JuceHeader.h>
#include "AppSettings.h"
#include "WFSLogger.h"

class UpdateChecker : private juce::Thread
{
public:
    using Callback = std::function<void (const juce::String& newVersion,
                                         const juce::String& releaseUrl)>;

    explicit UpdateChecker (Callback onUpdateAvailable)
        : juce::Thread ("UpdateChecker"),
          callback (std::move (onUpdateAvailable))
    {
    }

    ~UpdateChecker() override
    {
        stopThread (3000);
    }

    void checkNow()
    {
        startThread (juce::Thread::Priority::low);
    }

private:
    void run() override
    {
        if (threadShouldExit())
            return;

        juce::URL url ("https://api.github.com/repos/pob31/WFS-DIY/releases/latest");
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
            .withExtraHeaders (juce::String ("User-Agent: WFS-DIY/") + ProjectInfo::versionString)
            .withConnectionTimeoutMs (10000);

        auto stream = url.createInputStream (options);
        if (stream == nullptr)
        {
            WFSLogger::getInstance().logInfo ("Update check: network error (silent)");
            return;
        }

        auto responseText = stream->readEntireStreamAsString();

        if (threadShouldExit() || responseText.isEmpty())
            return;

        auto json = juce::JSON::parse (responseText);
        if (! json.isObject())
            return;

        auto tagName  = json.getProperty ("tag_name", "").toString();
        auto htmlUrl  = json.getProperty ("html_url", "").toString();

        if (tagName.isEmpty() || htmlUrl.isEmpty())
            return;

        auto remoteVersion = stripLeadingV (tagName);
        auto localVersion  = juce::String (ProjectInfo::versionString);

        if (! isNewerVersion (remoteVersion, localVersion))
        {
            WFSLogger::getInstance().logInfo (juce::String ("Update check: up-to-date (local ")
                                               + localVersion + ", remote " + remoteVersion + ")");
            return;
        }

        // Check if user already dismissed this version
        if (AppSettings::getUpdateDismissedVersion() == remoteVersion)
        {
            WFSLogger::getInstance().logInfo (juce::String ("Update check: v") + remoteVersion
                                               + " available but dismissed by user");
            return;
        }

        WFSLogger::getInstance().logInfo (juce::String ("Update check: v") + remoteVersion
                                           + " available (local " + localVersion + ")");

        auto cb        = callback;
        auto ver       = remoteVersion;
        auto releaseUrl = htmlUrl;
        juce::MessageManager::callAsync ([cb, ver, releaseUrl]() { cb (ver, releaseUrl); });
    }

    static juce::String stripLeadingV (const juce::String& s)
    {
        if (s.startsWithIgnoreCase ("v"))
            return s.substring (1);
        return s;
    }

    // Parses a segment like "0beta3" into numeric part (0) and beta number (3).
    // A segment with no beta suffix gets beta = -1 (i.e. release, sorts after all betas).
    struct SegmentInfo
    {
        int number = 0;
        int beta   = -1;  // -1 = release (no beta suffix)
    };

    static SegmentInfo parseSegment (const juce::String& seg)
    {
        SegmentInfo info;
        int betaPos = seg.indexOfIgnoreCase ("beta");

        if (betaPos >= 0)
        {
            info.number = seg.substring (0, betaPos).getIntValue();
            info.beta   = seg.substring (betaPos + 4).getIntValue();
        }
        else
        {
            info.number = seg.getIntValue();
            info.beta   = -1;
        }

        return info;
    }

    static bool isNewerVersion (const juce::String& remote, const juce::String& local)
    {
        auto remoteParts = juce::StringArray::fromTokens (remote, ".", "");
        auto localParts  = juce::StringArray::fromTokens (local, ".", "");

        for (int i = 0; i < juce::jmax (remoteParts.size(), localParts.size()); ++i)
        {
            auto r = parseSegment (i < remoteParts.size() ? remoteParts[i] : "0");
            auto l = parseSegment (i < localParts.size()  ? localParts[i]  : "0");

            if (r.number != l.number)
                return r.number > l.number;

            // Same numeric part — compare beta status.
            // -1 (release) beats any beta number, higher beta beats lower beta.
            if (r.beta != l.beta)
                return r.beta > l.beta;
        }

        return false;
    }

    Callback callback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UpdateChecker)
};
