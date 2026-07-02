"""Shared plumbing for the control-plane replay harnesses.

Stdlib-only (repo convention, see tools/fuzz/). Three drivers build on this:
  session_roundtrip.py  load fixture -> MCP session_save -> diff vs fixture
  osc_replay.py         scripted OSC writes -> OSCQuery read-back -> golden
  mcp_replay.py         scripted MCP transcript -> normalized -> golden

Design: docs/architecture/control-replay-harness.md.

Exit-code contract (same as tools/validation/kernel_hashes.py):
  0 = pass, 1 = mismatch, 2 = usage error, 3 = app failed to start.
"""

from __future__ import annotations

import ctypes
import difflib
import json
import os
import re
import shutil
import socket
import subprocess
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

EXIT_PASS = 0
EXIT_MISMATCH = 1
EXIT_USAGE = 2
EXIT_APP_START = 3

REPO_ROOT = Path(__file__).resolve().parents[3]
FIXTURE_DIR = Path(__file__).resolve().parent / "fixtures" / "golden-project"
GOLDENS_DIR = Path(__file__).resolve().parent / "goldens"

MCP_PORT = 7400
MCP_URL = f"http://127.0.0.1:{MCP_PORT}/mcp"
OSC_UDP_PORT = 8000          # networkRxUDPport in the fixture
OSCQUERY_HTTP_PORT = 5005    # networkOscQueryPort in the fixture

# Fixture project files the round-trip driver diffs (backups/ is ignored).
SECTION_FILES = ("system.xml", "network.xml", "inputs.xml",
                 "outputs.xml", "reverbs.xml")

EXE_CANDIDATES = (
    REPO_ROOT / "Builds/VisualStudio2022/x64/Release/App/WFS-DIY.exe",
    REPO_ROOT / "Builds/VisualStudio2022/x64/Debug/App/WFS-DIY.exe",
)


def find_exe(cli_value: str | None) -> Path:
    if cli_value:
        p = Path(cli_value)
        if not p.is_file():
            print(f"[common] --exe not found: {p}", file=sys.stderr)
            raise SystemExit(EXIT_USAGE)
        return p
    for c in EXE_CANDIDATES:
        if c.is_file():
            return c
    print("[common] no built WFS-DIY.exe found; build the app or pass --exe",
          file=sys.stderr)
    raise SystemExit(EXIT_APP_START)


# ---------------------------------------------------------------------------
# Process control — spawn, port polling, graceful WM_CLOSE, kill fallback
# ---------------------------------------------------------------------------

WM_CLOSE = 0x0010


def kill_stale_instances() -> None:
    """WFS-DIY is single-instance; a stale copy would swallow our launch
    (anotherInstanceStarted) and hold ports 7400/8000/8001."""
    result = subprocess.run(
        ["taskkill", "/IM", "WFS-DIY.exe", "/F"],
        capture_output=True, text=True)
    if result.returncode == 0:
        print("[common] killed stale WFS-DIY.exe instance(s)")
        time.sleep(1.0)  # let the OS release the listening sockets


def _enum_windows_for_pid(pid: int) -> list[int]:
    user32 = ctypes.windll.user32
    handles: list[int] = []

    @ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_void_p, ctypes.c_void_p)
    def cb(hwnd, _lparam):
        wnd_pid = ctypes.c_ulong()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(wnd_pid))
        if wnd_pid.value == pid and user32.IsWindowVisible(hwnd):
            handles.append(hwnd)
        return True

    user32.EnumWindows(cb, 0)
    return handles


class App:
    """One live WFS-DIY instance driven over MCP / OSC / OSCQuery."""

    def __init__(self, exe: Path, wfs_path: Path | None,
                 ai_enabled: bool = True):
        self.exe = exe
        self.wfs_path = wfs_path
        env = os.environ.copy()
        if ai_enabled:
            env["WFS_MCP_AI_ENABLED"] = "1"
        else:
            env.pop("WFS_MCP_AI_ENABLED", None)
        argv = [str(exe)]
        if wfs_path is not None:
            argv.append(str(wfs_path))
        self.proc = subprocess.Popen(
            argv, env=env,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
            cwd=str(exe.parent))
        self._rpc_id = 0

    # -- liveness -----------------------------------------------------------

    def alive(self) -> bool:
        return self.proc.poll() is None

    def wait_for_mcp(self, timeout: float = 90.0) -> None:
        """Poll POST /mcp with `initialize` until the dispatcher answers."""
        deadline = time.monotonic() + timeout
        last_err: object = None
        while time.monotonic() < deadline:
            if not self.alive():
                print(f"[common] app exited early (code {self.proc.poll()})",
                      file=sys.stderr)
                raise SystemExit(EXIT_APP_START)
            try:
                self.mcp("initialize", {
                    "protocolVersion": "2024-11-05",
                    "capabilities": {},
                    "clientInfo": {"name": "control-replay", "version": "1"},
                })
                self.mcp_notify("notifications/initialized")
                return
            except (urllib.error.URLError, ConnectionError, socket.timeout,
                    OSError) as exc:
                last_err = exc
                time.sleep(0.5)
        print(f"[common] MCP port {MCP_PORT} never answered: {last_err}",
              file=sys.stderr)
        self.force_kill()
        raise SystemExit(EXIT_APP_START)

    def wait_for_oscquery(self, timeout: float = 60.0,
                          port: int = OSCQUERY_HTTP_PORT) -> None:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            try:
                with urllib.request.urlopen(
                        f"http://127.0.0.1:{port}/?VALUE", timeout=2.0):
                    return
            except (urllib.error.URLError, ConnectionError, socket.timeout,
                    OSError):
                time.sleep(0.5)
        print(f"[common] OSCQuery HTTP port {port} never answered "
              "(is networkOscQueryEnabled=1 in the fixture?)", file=sys.stderr)
        self.force_kill()
        raise SystemExit(EXIT_APP_START)

    # -- MCP ----------------------------------------------------------------

    def _post(self, payload: dict) -> str:
        body = json.dumps(payload).encode("utf-8")
        req = urllib.request.Request(
            MCP_URL, data=body,
            headers={"Content-Type": "application/json"})
        with urllib.request.urlopen(req, timeout=30.0) as resp:
            return resp.read().decode("utf-8")

    def mcp(self, method: str, params: dict | None = None) -> dict:
        self._rpc_id += 1
        payload: dict = {"jsonrpc": "2.0", "id": self._rpc_id,
                         "method": method}
        if params is not None:
            payload["params"] = params
        text = self._post(payload)
        return json.loads(text)

    def mcp_notify(self, method: str) -> None:
        self._post({"jsonrpc": "2.0", "method": method})

    def tool(self, name: str, arguments: dict | None = None) -> dict:
        return self.mcp("tools/call",
                        {"name": name, "arguments": arguments or {}})

    def tool_confirmed(self, name: str, arguments: dict | None = None
                       ) -> tuple[dict, dict]:
        """Tier-2 two-step: first call yields awaiting_confirmation with a
        token; second call re-sends the same args + confirm=<token>.
        Returns (first_envelope, final_envelope)."""
        args = dict(arguments or {})
        first = self.tool(name, args)
        token = (envelope_result(first)
                 .get("tier_enforcement", {})
                 .get("confirmation_token"))
        if not token:
            # Not gated after all (e.g. operator window open) — done already.
            return first, first
        args["confirm"] = token
        final = self.tool(name, args)
        return first, final

    # -- shutdown -----------------------------------------------------------

    def close(self, timeout: float = 30.0) -> bool:
        """Graceful WM_CLOSE so cleanShutdown=true lands in the settings
        file and the project's system.xml is written. Returns True if the
        process exited gracefully."""
        if not self.alive():
            return True
        handles = []
        deadline = time.monotonic() + 5.0
        while not handles and time.monotonic() < deadline:
            handles = _enum_windows_for_pid(self.proc.pid)
            if not handles:
                time.sleep(0.25)
        for hwnd in handles:
            ctypes.windll.user32.PostMessageW(hwnd, WM_CLOSE, 0, 0)
        try:
            self.proc.wait(timeout=timeout)
            return True
        except subprocess.TimeoutExpired:
            print("[common] WARNING: app ignored WM_CLOSE; force-killing "
                  "(fixture/settings may be left dirty)", file=sys.stderr)
            self.force_kill()
            return False

    def force_kill(self) -> None:
        if self.alive():
            self.proc.kill()
            try:
                self.proc.wait(timeout=10.0)
            except subprocess.TimeoutExpired:
                pass


# ---------------------------------------------------------------------------
# OSC / OSCQuery helpers (reuse tools/fuzz primitives)
# ---------------------------------------------------------------------------

_FUZZ_DIR = REPO_ROOT / "tools" / "fuzz"
if str(_FUZZ_DIR) not in sys.path:
    sys.path.insert(0, str(_FUZZ_DIR))

from osc_codec import encode_message, encode_typed  # noqa: E402


class OSCSender:
    """UDP OSC sender (subset of tools/fuzz/osc_fuzz.py Sender — we only
    need well-formed UDP datagrams here)."""

    def __init__(self, host: str = "127.0.0.1", port: int = OSC_UDP_PORT,
                 delay: float = 0.01):
        self.addr = (host, port)
        self.delay = delay
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def send(self, address: str, args: list[tuple[str, object]]) -> None:
        self.sock.sendto(encode_message(address, args), self.addr)
        if self.delay > 0:
            time.sleep(self.delay)

    def close(self) -> None:
        self.sock.close()


def oscquery_get(path: str, port: int = OSCQUERY_HTTP_PORT,
                 host: str = "127.0.0.1"):
    """GET /<path>?VALUE -> the VALUE payload (usually a 1-element list)."""
    url = f"http://{host}:{port}{path}?VALUE"
    with urllib.request.urlopen(url, timeout=5.0) as resp:
        data = json.loads(resp.read().decode("utf-8"))
    if isinstance(data, dict) and "VALUE" in data:
        return data["VALUE"]
    return data


# ---------------------------------------------------------------------------
# Normalizers
# ---------------------------------------------------------------------------

_CREATED_RE = re.compile(r"^<!-- Created: .* -->\r?\n?", re.MULTILINE)
_WFS_APPVERSION_RE = re.compile(r'appVersion="[^"]*"')
_WFS_CREATEDDATE_RE = re.compile(r'createdDate="[^"]*"')


def normalize_xml_text(text: str) -> str:
    """Strip the volatile `<!-- Created: ... -->` header line
    (WFSFileManager::createXmlHeader) and normalize line endings."""
    text = text.replace("\r\n", "\n")
    return _CREATED_RE.sub("", text)


def normalize_wfs_manifest(text: str) -> str:
    """The .wfs manifest carries appVersion + createdDate that change per
    machine/run (WFSFileManager::createProjectManifest)."""
    text = text.replace("\r\n", "\n")
    text = _WFS_APPVERSION_RE.sub('appVersion="NORMALIZED"', text)
    return _WFS_CREATEDDATE_RE.sub('createdDate="NORMALIZED"', text)


# JSON envelope fields that are volatile across runs (design doc list).
_VOLATILE_KEYS = {
    "confirmation_token": "<TOKEN>",
    "expires_in_seconds": "<SECONDS>",
    "timestamp_iso": "<TIMESTAMP>",
}
_DROP_KEYS = {"notifications"}


def normalize_envelope(obj):
    """Recursively normalize a parsed MCP JSON-RPC envelope:
    - confirmation_token / expires_in_seconds replaced by placeholders
    - serverInfo.version, instructions normalized (release-varying)
    - _meta.ai_enabled / critical_actions_allowed normalized where they
      ride on tools/list (live operator state)
    - notifications[] dropped (cross-actor side-channel, timing-dependent)
    """
    if isinstance(obj, dict):
        out = {}
        for k in sorted(obj.keys()):
            v = obj[k]
            if k in _DROP_KEYS:
                continue
            if k in _VOLATILE_KEYS:
                out[k] = _VOLATILE_KEYS[k]
            elif k == "serverInfo" and isinstance(v, dict):
                v = dict(v)
                v["version"] = "<VERSION>"
                out[k] = normalize_envelope(v)
            elif k == "instructions" and isinstance(v, str):
                out[k] = "<INSTRUCTIONS>"
            else:
                out[k] = normalize_envelope(v)
        return out
    if isinstance(obj, list):
        return [normalize_envelope(x) for x in obj]
    return obj


def normalize_envelope_text(envelope: dict) -> dict:
    """Some envelopes carry a compact-JSON payload inside content[0].text
    that itself contains volatile bits (tokens). Normalize the embedded
    JSON too when it parses."""
    env = normalize_envelope(envelope)

    def fix(node):
        if isinstance(node, dict):
            for k, v in node.items():
                if k == "text" and isinstance(v, str):
                    try:
                        inner = json.loads(v)
                    except (json.JSONDecodeError, ValueError):
                        # Not JSON — normalize volatile substrings in prose
                        # (the tier-2 message embeds the token: `Re-call
                        # with \`confirm: "<uuid>"\` within 30 seconds.`)
                        s = re.sub(r'confirm:\s*"[0-9a-fA-F-]{36}"',
                                   'confirm: "<TOKEN>"', v)
                        node[k] = s
                        continue
                    node[k] = json.dumps(normalize_envelope(inner),
                                         sort_keys=True)
                else:
                    fix(v)
        elif isinstance(node, list):
            for x in node:
                fix(x)

    fix(env)
    return env


# ---------------------------------------------------------------------------
# Diffing / reporting
# ---------------------------------------------------------------------------

def unified_diff(expected: str, actual: str,
                 expected_label: str, actual_label: str) -> str:
    return "".join(difflib.unified_diff(
        expected.splitlines(keepends=True),
        actual.splitlines(keepends=True),
        fromfile=expected_label, tofile=actual_label))


def compare_or_update(golden_path: Path, actual_text: str,
                      update: bool, label: str) -> bool:
    """Golden-file contract shared by the drivers. Returns True on match
    (or after --update rewrote the golden)."""
    if update:
        golden_path.parent.mkdir(parents=True, exist_ok=True)
        golden_path.write_text(actual_text, encoding="utf-8", newline="\n")
        print(f"[{label}] golden updated: {golden_path}")
        return True
    if not golden_path.is_file():
        print(f"[{label}] golden missing: {golden_path} (run with --update)",
              file=sys.stderr)
        return False
    expected = golden_path.read_text(encoding="utf-8").replace("\r\n", "\n")
    actual = actual_text.replace("\r\n", "\n")
    if expected == actual:
        print(f"[{label}] PASS ({golden_path.name})")
        return True
    print(f"[{label}] MISMATCH vs {golden_path.name}:", file=sys.stderr)
    sys.stderr.write(unified_diff(expected, actual,
                                  str(golden_path), "<actual>"))
    return False


def copy_fixture_to_temp(work_root: Path) -> Path:
    """Fresh working copy of the golden project (never run the app on the
    committed fixture — saves would dirty the repo)."""
    if work_root.exists():
        shutil.rmtree(work_root)
    dest = work_root / FIXTURE_DIR.name
    shutil.copytree(FIXTURE_DIR, dest)
    return dest


def fixture_wfs(project_dir: Path) -> Path:
    return project_dir / f"{project_dir.name}.wfs"


def envelope_result(envelope: dict) -> dict:
    """The `result` member of a JSON-RPC response, or {}."""
    r = envelope.get("result")
    return r if isinstance(r, dict) else {}


def tool_payload(envelope: dict):
    """Parse the compact JSON inside result.content[0].text (tool results)."""
    result = envelope_result(envelope)
    content = result.get("content") or []
    if content and isinstance(content[0], dict):
        text = content[0].get("text", "")
        try:
            return json.loads(text)
        except (json.JSONDecodeError, ValueError):
            return text
    return None
