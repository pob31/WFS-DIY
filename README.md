# WFS-DIY

A Wave Field Synthesis WFS DIY project built with JUCE framework.

## Project Overview

This project is based on the Cycling74's Max8 Prototype found at https://wfs-diy.net

## Companion projects and documentation

- **[DAW Plugin Suite](Plugin/README.md)** — VST3 / AU / Standalone plugins (Master + 5 Track variants: Cartesian / Cylindrical / Spherical / ADM Cartesian / ADM Polar) that drive a running WFS-DIY session from any major DAW. Built from `Plugin/` as a standalone CMake subproject.
- **Android Remote** — companion Android app (WFS Control 2) for touch-based control over OSC. Lives in its own repo: <https://github.com/pob31/WFS_control_2>.
- **[Documentation/](Documentation/)** — reference material: `CLAUDE.md` (architecture / conventions), the `WFS-UI_*.csv` per-tab parameter inventories, `WFS-UI_arrayWizard.md` (array-wizard preset catalog and geometry formulas), `WFS-UI_plugins.md` (plugin setup guide for OSC / OSCQuery / ADM-OSC), and the `MCP/` specs for the in-progress MCP server.

## Features

- Cross-platform audio device management
- Real-time audio processing capabilities
- JUCE framework integration
- Multi-platform build support (Windows, macOS, Linux)

## Building the Project

### Prerequisites

**Windows:**
1. Install [Git for Windows](https://git-scm.com/download/win)
2. Install [Visual Studio 2022 Community](https://visualstudio.microsoft.com/) (free) — during install, select the **"Desktop development with C++"** workload

**macOS:**
1. Install [Xcode](https://apps.apple.com/app/xcode/id497799835) from the App Store (free)
2. Git is included with Xcode. If you need it before opening Xcode, run: `xcode-select --install`

**Linux:**
- GCC or Clang with development tools

### Step-by-step build

**1. Clone the repository**

Open a terminal and run:

```bash
git clone --recurse-submodules https://github.com/pob31/WFS-DIY.git
```

> **Important:** The `--recurse-submodules` flag is required. Without it, dependencies (JUCE, ASIO SDK) will be missing and the build will fail. If you already cloned without it, run:
> ```bash
> cd WFS-DIY
> git submodule update --init --recursive
> ```

**2. Open the project and build**

**Windows:**
1. Open `Builds/VisualStudio2022/WFS-DIY.sln` in Visual Studio
2. In the toolbar, set the platform to **x64** (not x86)
3. Select **Debug** or **Release** configuration
4. Build > Build Solution (or press Ctrl+Shift+B)
5. Debug > Start Debugging (or press F5) to run

**macOS:**
1. Open `Builds/MacOSX/WFS-DIY.xcodeproj` in Xcode
2. Select the **WFS-DIY** target and **My Mac** as destination
3. Product > Build (or press Cmd+B)
4. Product > Run (or press Cmd+R) to run
5. macOS will ask for microphone permission on first run — click Allow (required for audio input)

**Linux:**
1. Install build deps (Debian / Ubuntu):
   ```bash
   sudo apt install build-essential pkg-config libasound2-dev libfreetype6-dev \
       libfontconfig1-dev libgl1-mesa-dev libcurl4-openssl-dev libgtk-3-dev \
       libwebkit2gtk-4.1-dev libudev-dev
   ```
2. Build:
   ```bash
   cd Builds/LinuxMakefile && make CONFIG=Release -j$(nproc)
   ```
   The binary lands at `Builds/LinuxMakefile/build/WFS-DIY` along with `lang/` and `MCP/resources/` copied next to it.
3. **HID controller setup (one-time, optional):** to use Stream Deck, Xencelabs Quick Keys, or 3Dconnexion SpaceMouse without root, install the bundled udev rules:
   ```bash
   sudo cp assets/linux/50-wfs-diy.rules /etc/udev/rules.d/
   sudo udevadm control --reload-rules
   sudo udevadm trigger
   ```
   Then unplug and replug the device. The rules grant access via `uaccess` (active session) and the `plugdev` group fallback.

### GPU Audio (experimental)

WFS-DIY includes experimental support for GPU-accelerated audio processing using the GPU-Audio SDK. The SDK is integrated as a Git submodule at `ThirdParty/GPUAudioSDK`.

**For detailed setup instructions, see [GPU-Audio SDK Setup Guide](Documentation/GPU_AUDIO_SDK_SETUP.md)**

Quick start:
1. Initialize the submodule: `git submodule update --init --recursive`
2. Build the SDK following the setup guide
3. Set environment variables: `GPUAUDIO_PATH` and `GPUAUDIO_PROCESSOR_PATH`
4. In the app, choose `GPU InputBuffer (GPU Audio)` and enable processing

**Note**: The GPU Audio feature requires the [GPU Audio Platform](https://www.gpu.audio/sdk-binaries) to be installed separately.

## Running the Application

### Windows
- **Required**: [Microsoft Visual C++ Redistributable 2022 (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe) — may already be installed on your system
- **Recommended**: ASIO drivers for your audio interface (WASAPI is available as fallback)

### macOS
- No additional runtime dependencies — all required frameworks ship with macOS 10.13+
- Official `.pkg` releases are codesigned with Developer ID and notarized, so Gatekeeper opens them without any warning
- Grant microphone permission when prompted (required for audio input)

## AI Control (MCP Server)

WFS-DIY embeds a [Model Context Protocol](https://modelcontextprotocol.io/) (MCP) server so an AI assistant — Claude Desktop, Claude Code, ChatGPT with custom connectors, Cursor, or any MCP-capable client — can read and write the live session: move sources, rename channels, edit arrays, recall snapshots, run guided workflows over the full parameter surface.

The server is enabled by default. It starts automatically with the application and listens on **`http://127.0.0.1:7400/mcp`** (Streamable HTTP transport) bound to loopback only. If port 7400 is busy it falls back to the next three port numbers — the live URL is always shown by the **MCP URL** button in the Network tab.

### Operator controls (Network tab, "AI / MCP Server" row)

- **AI: ON / OFF** — master switch. When OFF every AI tool call is refused.
- **AI critical actions: blocked / ALLOWED** — destructive actions (delete snapshots, reset DSP, change channel counts) are blocked by default. Click to allow them for 10 minutes; the red fill drains as the window expires, then they auto-block again.
- **MCP URL button** — copies the live endpoint URL to the clipboard.
- **Open AI History** — scrollable timeline of every recent AI change, with per-row undo / redo. `Cmd/Ctrl + Alt + Z` and `Cmd/Ctrl + Alt + Y` undo / redo AI changes without affecting your manual edits (plain `Ctrl+Z` still works as usual on user actions).
- **Help "?" → Copy Config** — copies a ready-to-paste JSON snippet that always matches the live URL.

### Adding the server to your AI assistant

#### Claude Desktop (macOS / Windows)

1. Start WFS-DIY and confirm the **MCP URL** button shows a `http://...` URL (not "stopped").
2. In the Network tab, open the **AI / MCP Server** help card and click **Copy Config**. You'll get a snippet like:
   ```json
   {
     "mcpServers": {
       "wfs-diy": {
         "url": "http://127.0.0.1:7400/mcp"
       }
     }
   }
   ```
3. In Claude Desktop: **Settings → Developer → Edit Config**. This opens `claude_desktop_config.json`.
4. Paste the snippet. If you already have an `mcpServers` block, merge the `"wfs-diy"` entry into it rather than duplicating the parent key.
5. Save and restart Claude Desktop. The server appears as **wfs-diy** in the tools menu.

#### Claude Code (CLI)

With WFS-DIY running, copy the URL shown by the Network tab's **MCP URL** button, then run:
```bash
claude mcp add wfs-diy http://127.0.0.1:7400/mcp -t http
```
Replace the URL with whatever the button shows if the server has fallen back to a different port. Verify with `claude mcp list` and ask Claude to "list inputs" to confirm the connection.

#### Other MCP-capable clients (ChatGPT custom connectors, Cursor, Continue, etc.)

Any client that supports the **Streamable HTTP** MCP transport can connect to the same endpoint. Configure a new server with:

- **Transport:** Streamable HTTP (a single endpoint, not the legacy HTTP+SSE pair)
- **URL:** the value of the MCP URL button (default `http://127.0.0.1:7400/mcp`)
- **Authentication:** none

Refer to your client's MCP configuration documentation for the exact UI path.

### Connecting from a different machine

The server binds to loopback (`127.0.0.1`) by default — only an AI client running on the same computer as WFS-DIY can reach it. To allow a remote AI client, switch the bind scope to the selected network interface in the Network tab, then use that machine's LAN IP in the URL (e.g. `http://192.168.1.20:7400/mcp`). Treat this as a local-trust setup: there is no authentication in v1, so do not expose port 7400 to the public internet.

### Troubleshooting

- **MCP URL button shows "stopped".** The server failed to bind. Open the Network Log window to see the reason — usually another process is holding port 7400. The server tries the next three ports automatically, so re-copy the URL once it comes up.
- **AI assistant reports "tool call refused".** Check the **AI: ON / OFF** toggle and the **AI critical actions** gate. Tier-3 (destructive) tools require both to be permissive.
- **Endpoint URL changed.** Switching network interface or hitting a port fallback updates the URL. The MCP URL button always reflects the live value — re-copy it and update your client config.
- **AI looks out of sync after you moved a fader by hand.** Expected. Manual edits take precedence; the AI is notified and re-reads state before its next action.

## Development

This project uses Git for version control and is designed for cross-platform development. Make sure to:

- Test changes on multiple platforms when possible
- Keep build files synchronized across platforms
- Follow JUCE coding conventions

## Contributing

When contributing to this project:

1. Make sure your changes compile on your target platform
2. Test functionality before committing
3. Update documentation as needed
4. Follow the existing code style

## License

This project is licensed under the GNU General Public License v3.0 (GPL-3.0).

Copyright (c) 2026 Pierre-Olivier Boulant

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

### GPL v3 Key Principles

- **Freedom to use**: You can run the software for any purpose
- **Freedom to study**: You can examine and modify the source code
- **Freedom to distribute**: You can share copies of the software
- **Freedom to distribute modifications**: You can share your modified versions

**Important**: Any derivative works must also be licensed under GPL v3, ensuring the software remains free and open source.
