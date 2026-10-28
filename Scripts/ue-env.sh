#!/bin/sh
# [2026-10-27 T-002] Shared UE paths for every Scripts/*.sh gate.
# UE_ROOT may be overridden; the default is the verified source build on this Mac.
UE_ROOT="${UE_ROOT:-/Users/auston/Development Projects/UnrealEngine}"
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UPROJECT="$PROJECT_ROOT/UnderTheMountain.uproject"
UE_BUILD="$UE_ROOT/Engine/Build/BatchFiles/Mac/Build.sh"
UE_EDITOR_CMD="$UE_ROOT/Engine/Binaries/Mac/UnrealEditor-Cmd"
