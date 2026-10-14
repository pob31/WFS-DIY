#pragma once

#include <deque>
#include <functional>
#include <mutex>
#include <juce_events/juce_events.h>
#include <juce_osc/juce_osc.h>
#include "../../../spatcore/control/osc/OSCParser.h"

namespace wfs::plugin
{
    /** The Master's inbound ADM-OSC: plain /adm/obj/... messages on one UDP port.

        juce::OSCReceiver parsed every datagram with no limit on bundle nesting:
        one 64 KB packet nested about 3,000 deep recursed past the stack of its
        "JUCE OSC server" thread, deep-copying each level's subtree on the way
        (re-audit 2026-09-29, B3). The Master never handled bundles - it only
        listened for messages - and the app sends ADM-OSC as plain messages, so
        a bundle is dropped here unread. A message is parsed by spatcore's
        OSCParser, which reads it linearly.

        Datagrams are read on this receiver's own thread. Parsed messages are
        queued (at most maxQueued, oldest dropped first) and delivered on the
        message thread through onMessage, as the old MessageLoopCallback
        listener was. The destructor joins the thread and cancels a pending
        delivery, so no callback outlives the receiver.
    */
    class AdmOscReceiver final : private juce::Thread,
                                 private juce::AsyncUpdater
    {
    public:
        static constexpr size_t maxQueued = 1024;

        /** Called on the message thread for each /adm/obj/... message. */
        std::function<void (const juce::OSCMessage&)> onMessage;

        AdmOscReceiver() : juce::Thread ("WFS-DIY ADM-OSC rx") {}

        ~AdmOscReceiver() override
        {
            disconnect();
            cancelPendingUpdate();
        }

        /** Binds `port` on all interfaces and starts reading. */
        bool connect (int port)
        {
            disconnect();

            auto newSocket = std::make_unique<juce::DatagramSocket> (false);
            if (! newSocket->bindToPort (port))
                return false;

            socket = std::move (newSocket);
            startThread();
            return true;
        }

        void disconnect()
        {
            if (socket == nullptr)
                return;

            signalThreadShouldExit();
            socket->shutdown();
            stopThread (2000);
            socket.reset();

            const std::lock_guard<std::mutex> l (queueLock);
            queue.clear();
        }

        bool isConnected() const noexcept { return socket != nullptr; }

    private:
        void run() override
        {
            juce::HeapBlock<char> buffer (65536);

            while (! threadShouldExit())
            {
                const int ready = socket->waitUntilReady (true, 100);
                if (ready < 0)
                    return;
                if (ready == 0)
                    continue;

                const int bytes = socket->read (buffer.getData(), 65536, false);
                if (bytes < 4 || buffer[0] != '/')
                    continue;   // a bundle ('#') or junk: never handled, dropped unread

                try
                {
                    int pos = 0;
                    auto message = spatcore::control::osc::OSCParser::parseMessage (buffer.getData(), bytes, pos);
                    if (! message.getAddressPattern().toString().startsWith ("/adm/obj/"))
                        continue;

                    {
                        const std::lock_guard<std::mutex> l (queueLock);
                        if (queue.size() >= maxQueued)
                            queue.pop_front();
                        queue.push_back (std::move (message));
                    }
                    triggerAsyncUpdate();
                }
                catch (const juce::OSCFormatError&) {}
            }
        }

        void handleAsyncUpdate() override
        {
            std::deque<juce::OSCMessage> batch;
            {
                const std::lock_guard<std::mutex> l (queueLock);
                batch.swap (queue);
            }

            for (const auto& message : batch)
                if (onMessage != nullptr)
                    onMessage (message);
        }

        std::unique_ptr<juce::DatagramSocket> socket;
        std::mutex queueLock;
        std::deque<juce::OSCMessage> queue;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdmOscReceiver)
    };
}
