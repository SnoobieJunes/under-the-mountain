#!/bin/sh
# [2026-10-27 T-002] Build the editor target (compiles the gameplay module).
set -eu
. "$(dirname "$0")/ue-env.sh"
"$UE_BUILD" UnderTheMountainEditor Mac Development -Project="$UPROJECT" -WaitMutex -NoHotReload "$@"
echo "OK: UnderTheMountainEditor built"
