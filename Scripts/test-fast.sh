#!/bin/sh
# [2026-10-27 T-002] Fast gate: build editor target, then run UnderTheMountain
# automation tests headless. This is the inner loop; gate.sh is the full gate.
set -eu
. "$(dirname "$0")/ue-env.sh"
Scripts/build.sh
"$UE_EDITOR_CMD" "$UPROJECT" -ExecCmds="Automation RunTests UnderTheMountain; Quit" \
  -TestExit="Automation Test Queue Empty" -NullRHI -Unattended -NoSplash -NoSound
echo "OK: automation tests completed (see ~/Library/Logs/Unreal Engine/UnderTheMountainEditor/)"
