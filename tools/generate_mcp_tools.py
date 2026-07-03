#!/usr/bin/env python3
"""Generate the MCP tool surface from WFS-DIY's WFS-UI_*.csv source CSVs.

Thin entry-point wrapper (spatcore Phase 4d — mirrors the
tools/windows/build-gpu-plugins.ps1 precedent): the app-agnostic
generator core moved to spatcore/tools/codegen/generate_mcp_tools.py and
the WFS-DIY vocabulary/config to tools/mcp/wfs_codegen_config.py. This
file keeps the historical invocation — `python tools/generate_mcp_tools.py`
from the repo root, as run by the VS prebuild step in WFS-DIY.jucer /
Builds/VisualStudio2022/WFS-DIY_App.vcxproj — working unchanged, with
the same CLI (see tools/mcp/README.md).

Importing this module yields the *configured core module itself* (via
sys.modules self-replacement), so existing importers —
tools/mcp/test_generate_mcp_tools.py, tools/mcp/populate_tier_column.py,
tools/audit_param_bounds.py — keep `import generate_mcp_tools as g`
unchanged, including attribute writes like `g._REAL_FAMILY_STEMS = ...`
which must land on the core's own globals.

Note on the prebuild's failure behaviour: the "silently no-op when
python is missing" trap lives entirely in the .jucer/.vcxproj command
line (`where python >nul 2>&1 && python ... & exit /b 0`), not here.
This wrapper propagates the generator's exit code, so a CI job that
wants a loud gate simply runs `python tools/generate_mcp_tools.py`
directly and checks the exit status.
"""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parent.parent
_CORE_PATH = _REPO_ROOT / "spatcore" / "tools" / "codegen" / "generate_mcp_tools.py"
_CONFIG_PATH = _REPO_ROOT / "tools" / "mcp" / "wfs_codegen_config.py"


def _load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise ImportError(f"cannot load {name} from {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


_core = _load_module("spatcore_generate_mcp_tools", _CORE_PATH)
_config = _load_module("wfs_codegen_config", _CONFIG_PATH)
_core.configure(_config)

# Replace this module with the configured core so importers see the real
# module object (functions, tables, and mutable globals alike).
sys.modules[__name__] = _core

if __name__ == "__main__":
    sys.exit(_core.main())
