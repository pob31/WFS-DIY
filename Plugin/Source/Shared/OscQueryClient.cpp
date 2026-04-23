#include "OscQueryClient.h"

namespace wfs::plugin
{
    OscQueryClient::OscQueryClient() = default;

    OscQueryClient::~OscQueryClient()
    {
        disconnect();
    }

    void OscQueryClient::setOscCallback (OscCallback cb) { oscCallback = std::move (cb); }

    juce::String OscQueryClient::getLastHostInfo() const
    {
        return cachedHostInfo;
    }

    void OscQueryClient::setState (State s)
    {
        // State is an atomic — consumers poll getState() on their own schedule.
        // Avoid async callbacks here because the callback could outlive the
        // owning AudioProcessor during host-scan teardown and dangle.
        state.store (s);
    }

    bool OscQueryClient::httpGet (const juce::String& pathAndQuery, juce::String& outBody)
    {
        // juce::URL would re-encode OSCQuery's bare `?HOST_INFO` query as
        // `?HOST_INFO=` (key-with-empty-value form), which the server
        // doesn't match. Send the request over a raw TCP socket so the
        // path+query reach the server exactly as written.
        juce::String host;
        int port = 0;
        {
            std::lock_guard<std::mutex> sl (lock);
            host = currentHost;
            port = currentHttpPort;
        }

        juce::StreamingSocket socket;
        if (! socket.connect (host, port, 2000))
            return false;

        const juce::String request =
            "GET " + pathAndQuery + " HTTP/1.1\r\n"
            "Host: " + host + ":" + juce::String (port) + "\r\n"
            "Connection: close\r\n"
            "User-Agent: WFS-DIY-Plugin/1.0\r\n"
            "\r\n";

        const auto utf8 = request.toRawUTF8();
        const int reqLen = static_cast<int> (std::strlen (utf8));
        if (socket.write (utf8, reqLen) != reqLen)
            return false;

        juce::MemoryBlock raw;
        char buffer[4096];
        const juce::uint32 deadline = juce::Time::getMillisecondCounter() + 3000;

        while (juce::Time::getMillisecondCounter() < deadline)
        {
            if (socket.waitUntilReady (true, 500) != 1)
                continue;
            const int n = socket.read (buffer, sizeof (buffer), false);
            if (n <= 0) break;
            raw.append (buffer, static_cast<size_t> (n));
        }
        socket.close();

        if (raw.getSize() == 0)
            return false;

        const auto responseStr = juce::String::fromUTF8 (
            static_cast<const char*> (raw.getData()),
            static_cast<int> (raw.getSize()));

        const int headerEnd = responseStr.indexOf ("\r\n\r\n");
        if (headerEnd < 0)
            return false;

        const auto statusLine = responseStr.upToFirstOccurrenceOf ("\r\n", false, false);
        const int statusCode = statusLine.fromFirstOccurrenceOf (" ", false, false)
                                         .upToFirstOccurrenceOf (" ", false, false)
                                         .getIntValue();
        if (statusCode < 200 || statusCode >= 300)
            return false;

        outBody = responseStr.substring (headerEnd + 4);
        return outBody.isNotEmpty();
    }

    bool OscQueryClient::connect (const juce::String& host, int httpPort)
    {
        disconnect();

        {
            std::lock_guard<std::mutex> sl (lock);
            currentHost     = host;
            currentHttpPort = httpPort;
        }

        setState (State::Connecting);

        juce::String body;
        if (! httpGet ("/?HOST_INFO", body))
        {
            setState (State::Error);
            return false;
        }
        cachedHostInfo = body;

        setState (State::Handshaking);

        ws = std::make_unique<SimpleWebSocketClient>();
        ws->addWebSocketListener (this);
        // SocketClientBase (Simple-Web-Server) parses this as "host:port/path";
        // passing a `ws://` prefix would make it read "ws:" as the host.
        ws->start (host + ":" + juce::String (httpPort));
        return true;
    }

    void OscQueryClient::disconnect()
    {
        if (ws != nullptr)
        {
            ws->removeWebSocketListener (this);
            ws->stop();
            ws.reset();
        }

        {
            std::lock_guard<std::mutex> sl (lock);
            subscribedPaths.clear();
        }

        setState (State::Idle);
    }

    void OscQueryClient::sendCommand (const juce::String& command, const juce::String& path)
    {
        if (ws == nullptr || ! ws->isConnected)
            return;

        auto obj = new juce::DynamicObject();
        obj->setProperty ("COMMAND", command);
        obj->setProperty ("DATA",    path);
        const juce::String payload = juce::JSON::toString (juce::var (obj), true);
        ws->send (payload);
    }

    bool OscQueryClient::listen (const juce::String& oscPath)
    {
        {
            std::lock_guard<std::mutex> sl (lock);
            if (std::find (subscribedPaths.begin(), subscribedPaths.end(), oscPath) != subscribedPaths.end())
                return true;
            subscribedPaths.push_back (oscPath);
        }
        sendCommand ("LISTEN", oscPath);
        return true;
    }

    bool OscQueryClient::ignore (const juce::String& oscPath)
    {
        {
            std::lock_guard<std::mutex> sl (lock);
            subscribedPaths.erase (std::remove (subscribedPaths.begin(),
                                                subscribedPaths.end(),
                                                oscPath),
                                   subscribedPaths.end());
        }
        sendCommand ("IGNORE", oscPath);
        return true;
    }

    void OscQueryClient::connectionOpened()
    {
        setState (State::Ready);

        std::vector<juce::String> toResubscribe;
        {
            std::lock_guard<std::mutex> sl (lock);
            toResubscribe = subscribedPaths;
        }
        for (auto& path : toResubscribe)
            sendCommand ("LISTEN", path);
    }

    void OscQueryClient::messageReceived (const juce::String& /*message*/)
    {
        // Server may send status JSON on the WebSocket; currently unused.
    }

    void OscQueryClient::dataReceived (const juce::MemoryBlock& data)
    {
        juce::String path;
        float value = 0.0f;
        if (! decodeOscPacket (data, path, value))
            return;
        if (oscCallback)
            oscCallback (path, value);
    }

    void OscQueryClient::connectionClosed (int /*status*/, const juce::String& /*reason*/)
    {
        setState (State::Idle);
    }

    void OscQueryClient::connectionError (const juce::String& /*message*/)
    {
        setState (State::Error);
    }

    namespace
    {
        uint32_t readBigEndianU32 (const uint8_t* p) noexcept
        {
            return (uint32_t (p[0]) << 24) | (uint32_t (p[1]) << 16)
                 | (uint32_t (p[2]) << 8)  |  uint32_t (p[3]);
        }

        int32_t readBigEndianI32 (const uint8_t* p) noexcept
        {
            return static_cast<int32_t> (readBigEndianU32 (p));
        }

        float readBigEndianFloat (const uint8_t* p) noexcept
        {
            const uint32_t raw = readBigEndianU32 (p);
            float f;
            std::memcpy (&f, &raw, sizeof (float));
            return f;
        }

        size_t paddedLength (size_t n) noexcept
        {
            return (n + 4u) & ~size_t (3);
        }
    }

    bool OscQueryClient::decodeOscPacket (const juce::MemoryBlock& data,
                                          juce::String& outPath,
                                          float& outValue)
    {
        const auto* bytes = static_cast<const uint8_t*> (data.getData());
        const size_t size = data.getSize();
        if (bytes == nullptr || size < 8)
            return false;

        size_t addrLen = 0;
        while (addrLen < size && bytes[addrLen] != 0)
            ++addrLen;
        if (addrLen == 0 || addrLen >= size)
            return false;

        outPath = juce::String::fromUTF8 (reinterpret_cast<const char*> (bytes), static_cast<int> (addrLen));
        const size_t addrBlock = paddedLength (addrLen);
        if (addrBlock + 4 > size)
            return false;

        const char* tagStart = reinterpret_cast<const char*> (bytes + addrBlock);
        if (tagStart[0] != ',')
            return false;
        const char typeCode = tagStart[1];

        const size_t tagBlock = paddedLength (1 + 1);
        const size_t argPos   = addrBlock + tagBlock;
        if (argPos + 4 > size)
            return false;

        switch (typeCode)
        {
            case 'f':
                outValue = readBigEndianFloat (bytes + argPos);
                return true;
            case 'i':
                outValue = static_cast<float> (readBigEndianI32 (bytes + argPos));
                return true;
            case 'T':
                outValue = 1.0f;
                return true;
            case 'F':
                outValue = 0.0f;
                return true;
            default:
                return false;
        }
    }
}
