#!/bin/sh
# [2026-10-27 T-002] Bootstrap check: pinned toolchain present before any build.
set -eu
. "$(dirname "$0")/ue-env.sh"
[ -d "$UE_ROOT/Engine" ] || { echo "FAIL: engine not found at $UE_ROOT (set UE_ROOT)"; exit 1; }
[ -x "$UE_BUILD" ] || { echo "FAIL: Build.sh missing under $UE_ROOT"; exit 1; }
[ -x "$UE_EDITOR_CMD" ] || { echo "FAIL: UnrealEditor-Cmd missing (engine not built)"; exit 1; }
grep -q '"MajorVersion": 5' "$UE_ROOT/Engine/Build/Build.version" || { echo "FAIL: not a UE5 engine"; exit 1; }
grep -q '"MinorVersion": 8' "$UE_ROOT/Engine/Build/Build.version" || { echo "FAIL: engine is not 5.8.x (pinned)"; exit 1; }
xcodebuild -version >/dev/null 2>&1 || { echo "FAIL: Xcode not available"; exit 1; }
echo "OK: UE $(grep -A2 MajorVersion "$UE_ROOT/Engine/Build/Build.version" | tr -d '\n\t ' | sed 's/,/./g') at $UE_ROOT"
xcodebuild -version | head -n 1
echo "OK: project $UPROJECT"
