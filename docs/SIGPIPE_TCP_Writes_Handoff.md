# Handoff: a TCP peer that hangs up can kill WFS-DIY, or the DAW hosting its Master plugin

*Written 2026-11-11 by a Claude session working in the OSCQuery Automation Plugin
(`../OSCQuery-Automation-plugin`), for the author and for whichever Claude session picks this up in WFS-DIY.
WFS-DIY was read at `17f9ad7f`; nothing in WFS-DIY was changed, built or run. The same defect was found,
measured and fixed in the OSCQuery Automation Plugin the same day, and go.dot has it too (go.dot's handoff:
`docs/handoffs/2026-11-11-fixes-from-the-oscquery-plugin.md` in that repository).*

## In one paragraph

On macOS and Linux, writing with `juce::StreamingSocket::write` to a TCP connection the other end has closed
raises `SIGPIPE`. Unless the process has arranged otherwise, that signal ends it at once: no crash report, no
exception, nothing logged. WFS-DIY writes that way in three places. **The worst is in the Master plugin:**
its HTTP request to the WFS-DIY app could take down the DAW hosting it (REAPER, Live, Bitwig, GarageBand)
if the app quits or restarts at the wrong instant. **The other two are in the app:** the TCP OSC sender and
the MQTT tracking client, where a target or broker that goes away can end WFS-DIY mid-show. Windows has no
such signal and is not affected. Two fixes: one line in the app's `initialise()`, and a write that cannot
raise the signal in the plugin, which must not touch its host's signal handling.

## What happens

`send()` on a TCP socket whose peer has closed raises `SIGPIPE`. `juce::StreamingSocket::write` is a plain
`send()` with no flag (`juce_core/network/juce_Socket.cpp`), and JUCE sets no socket option against it. JUCE
installs a `SIGPIPE` handler only for its named pipes, and nothing in WFS-DIY handles the signal. Usually
the first write after the peer has gone still goes through and draws a reset; the next one raises the
signal.

A sign to look for in user reports: **WFS-DIY gone without a crash report, its session log
(`WFSLogger`) stopping mid-session with no line about shutting down.**

## Where WFS-DIY is exposed

| File | Side | What is on the other end | How likely |
|---|---|---|---|
| `Plugin/Source/Shared/OscQueryClient.cpp:57`, in `OscQueryClient::httpGet` | **Master plugin** (VST3, AU, LV2, Standalone) | The WFS-DIY app, asked for `HOST_INFO` (`:118`), a value (`:196`) or a namespace (`:221`) | Low: the app must close between the plugin's connection and its write. When it happens, **the DAW dies**. |
| `Source/Network/OSCConnection.cpp:382`, in `sendWithLengthPrefix` | App | A TCP OSC target | Real whenever TCP targets are used: a target that restarts or drops the connection ends WFS-DIY on the next send. |
| `Source/Network/TrackingMQTTReceiver.cpp:240`, `:271`, `:396`, `:402` | App | The MQTT broker of a tracking system | `:396` (the keepalive `PINGREQ`) is the likely one: a broker that went away gets the next ping. `:402` (`DISCONNECT`) can end the app as it quits. |

Not exposed:

- **The OSCQuery server** (`Source/Network/OSCQueryServer.*`) and the Master plugin's websocket run on
  `juce_simpleweb` over asio, which sets `SO_NOSIGPIPE` / `MSG_NOSIGNAL` itself.
- **The MCP server** (spatcore `control/mcp/MCPTransport`) is also on `juce_simpleweb`.
- **spatcore's `OSCTCPReceiver`** only reads.
- **UDP** cannot raise `SIGPIPE`.

## Measured, in the OSCQuery Automation Plugin

That plugin's HTTP client had the same write. On the Mac mini, its test program vanished once in about a
dozen full runs, in tests that restart their mock device: no summary, no error, no macOS crash report. On the
Linux NUC, a test that writes to a peer that has closed, using `juce::StreamingSocket::write`, ends the test
program with **exit status 141 (128 + 13, `SIGPIPE`) and prints nothing**. With the fix it passes, three runs
out of three. This was not reproduced in WFS-DIY; the mechanism is the same JUCE call.

## Fixes

### The app: ignore the signal once

The app owns its process, so it can ignore `SIGPIPE` for good at the top of
`WFSDIYApplication::initialise` (`Source/Main.cpp:69`):

```cpp
#if ! JUCE_WINDOWS
 #include <csignal>
#endif

void initialise (const juce::String& commandLine) override
{
   #if ! JUCE_WINDOWS
    // A TCP peer that hangs up must cost a failed write, not the whole application:
    // juce::StreamingSocket::write raises SIGPIPE on macOS and Linux.
    std::signal (SIGPIPE, SIG_IGN);
   #endif

    // Start session logger before anything else
    WFSLogger::getInstance().initialise();
    ...
```

That covers `OSCConnection`, the MQTT client and anything added later. A write to a closed connection then
returns -1. `sendWithLengthPrefix` already returns false on a short write. The MQTT client checks the
results of `:240` and `:271` but not those of `:396` and `:402`. After this fix that is harmless: the read
loop sees the closed socket and reconnects.

### The Master plugin: a write that cannot raise the signal

A plugin must not change its host's signal handlers, so the plugin cannot use the app's fix. Its one write
in `OscQueryClient::httpGet` (`Plugin/Source/Shared/OscQueryClient.cpp:57`) goes through a write that never
raises the signal:

```cpp
#if ! JUCE_WINDOWS
 #include <sys/socket.h>
 #include <sys/types.h>
#endif

namespace
{
    // juce::StreamingSocket::write without SIGPIPE: MSG_NOSIGNAL on Linux, SO_NOSIGPIPE on macOS.
    int writeNoSignal (juce::StreamingSocket& socket, const void* data, int size) noexcept
    {
       #if JUCE_WINDOWS
        return socket.write (data, size);
       #else
        const auto handle = socket.getRawSocketHandle();

        if (handle < 0 || ! socket.isConnected())
            return -1;

        #if defined (MSG_NOSIGNAL)
        return (int) ::send (handle, data, (size_t) size, MSG_NOSIGNAL);
        #else
        const int on = 1;
        (void) ::setsockopt (handle, SOL_SOCKET, SO_NOSIGPIPE, &on, (socklen_t) sizeof (on));
        return (int) ::send (handle, data, (size_t) size, 0);
        #endif
       #endif
    }
}

// in httpGet:
        if (writeNoSignal (socket, utf8, reqLen) != reqLen)
            return false;
```

It covers every format, the Standalone build included. If `OSCConnection` or the MQTT client ever go into
the plugin, they need the same write; the app's one-line fix does not protect a plugin.

The OSCQuery Automation Plugin's version of this function is `lib/oscquery/oscquery/net/WriteNoSignal.h`,
with tests in `lib/oscquery/tests/WriteNoSignalTests.cpp` (its pull request #9).

## To test

1. **A unit test.** Open a listener of the test's own and connect to it. Close the accepted end, then write
   until the write fails. Without the fix the test program exits with 141 on Linux and nothing more is
   printed; with it, the write returns -1 and the test passes. The OSCQuery plugin's first case in
   `WriteNoSignalTests.cpp` does exactly this.
2. **By hand, the app.** Configure a TCP OSC target pointing at `nc -l <port>` and send to it. Stop `nc` and
   send again. Without the fix WFS-DIY disappears and its log stops; with it, the target shows as
   disconnected.

## The rest of that session's findings, and why they do not apply

- **mDNS answers without pacing, and a browser that keeps old records across a restart.** WFS-DIY has no
  mDNS code. When it gains DNS-SD (the OSCQuery plugin's devplan, Phase 8, plans it as companion work), it
  should take the paced version: `lib/oscquery/oscquery/mdns/AnswerPacer.h` and the changes around it,
  OSCQuery plugin pull request #4. The unpaced one sent 11,600 multicast datagrams in a second when loaded
  in many processes at once.
- **An HTTP GET that ignores `Content-Length`.** The Master plugin's client only talks to WFS-DIY's own
  server, which closes after each reply.
