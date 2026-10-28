#!/bin/sh
# [2026-10-27 T-002] Open the project in Unreal Editor (GUI smoke path).
set -eu
. "$(dirname "$0")/ue-env.sh"
exec "$UE_ROOT/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" "$UPROJECT" "$@"
