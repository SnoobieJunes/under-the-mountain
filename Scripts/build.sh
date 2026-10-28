#!/bin/sh
# [2026-10-27 T-002] Build the editor target (compiles the gameplay module).
set -eu
. "$(dirname "$0")/ue-env.sh"
"$UE_BUILD" UnderTheMountainEditor Mac Development -Project="$UPROJECT" -WaitMutex -NoHotReload -NoUba "$@"
# [2026-10-27 T-024] -NoUba: the shared UBA cache served compile actions to a
# cold clone without materialising .o files (link failed, retry passed). Gates
# must be deterministic on first pass; direct clang is the honest cost.
echo "OK: UnderTheMountainEditor built"
