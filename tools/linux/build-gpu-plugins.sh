#!/usr/bin/env bash
# Thin wrapper — the script moved to spatcore/tools/gpu/ (Phase 2 of the
# spatcore extraction). Kept so existing docs/CI invocations keep working.
exec "$(dirname "$0")/../../spatcore/tools/gpu/build-gpu-plugins.sh" "$@"
