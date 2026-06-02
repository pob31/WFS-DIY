#pragma once

#include <JuceHeader.h>
#include <thread>
#include <atomic>
#include <memory>
#include "AppSettings.h"
#include "WFSLogger.h"

/**
 * UpdateChecker - background check for a newer GitHub release.
 *
 * The check runs on a DETACHED thread that is never joined. On Windows a blocking
 * WinINet request cannot be reliably interrupted from another thread (cancel() and
 * the connection timeout are ignored for synchronous handles), so joining at
 * shutdown could hang for the OS TCP timeout and trip Thread::stopThread's
 * force-kill assertion. Instead a shared "alive" token is cleared on destruction:
 * the worker then performs no further app interaction, and if it is still blocked
 * it is reaped cleanly by the OS at process exit. All app interaction (logging and
 * the update callback) is marshalled to the message thread via callAsync, where it
 * cannot race the logger's or the app's teardown.
 */
class UpdateChecker
{
public:
    using Callback = std::function<void (const juce::String& newVersion,
                                         const juce::String& releaseUrl)>;

    explicit UpdateChecker (Callback onUpdateAvailable)
        : callback (std::move (onUpdateAvailable))
    {
    }

    ~UpdateChecker()
    {
        // Abandon the background check without joining it (see class comment).
        *alive = false;
    }

    void checkNow()
    {
        auto aliveToken = alive;     // shared with the detached worker
        auto cb = callback;
        std::thread ([aliveToken, cb]() { runCheck (aliveToken, cb); }).detach();
    }

private:
    using AliveToken = std::shared_ptr<std::atomic<bool>>;

    static void runCheck (AliveToken alive, Callback callback)
    {
        juce::URL url ("https://api.github.com/repos/pob31/WFS-DIY/releases/latest");
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
            .withExtraHeaders (juce::String ("User-Agent: WFS-DIY/") + ProjectInfo::versionString)
            .withConnectionTimeoutMs (10000);

        auto stream = url.createInputStream (options);

        if (! alive->load())   // app went away while we were connecting
            return;

        if (stream == nullptr)
        {
            logAsync (alive, "Update check: network error (silent)");
            return;
        }

        auto responseText = stream->readEntireStreamAsString();

        if (! alive->load() || responseText.isEmpty())
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
            logAsync (alive, juce::String ("Update check: up-to-date (local ")
                              + localVersion + ", remote " + remoteVersion + ")");
            return;
        }

        // Check if user already dismissed this version
        if (AppSettings::getUpdateDismissedVersion() == remoteVersion)
        {
            logAsync (alive, juce::String ("Update check: v") + remoteVersion
                              + " available but dismissed by user");
            return;
        }

        logAsync (alive, juce::String ("Update check: v") + remoteVersion
                          + " available (local " + localVersion + ")");

        juce::MessageManager::callAsync ([alive, callback, remoteVersion, htmlUrl]()
        {
            if (alive->load() && callback != nullptr)
                callback (remoteVersion, htmlUrl);
        });
    }

    // Log on the message thread, where it cannot race WFSLogger::shutdown().
    static void logAsync (AliveToken alive, juce::String message)
    {
        juce::MessageManager::callAsync ([alive, message]()
        {
            if (alive->load())
                WFSLogger::getInstance().logInfo (message);
        });
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
    AliveToken alive { std::make_shared<std::atomic<bool>> (true) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UpdateChecker)
};
