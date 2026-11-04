#!/bin/sh
# [2026-11-04 T-123] Single-frame dragon capture: build the game target,
# launch -game at a proof spawn, fire one HighResShot, quit, and move the
# shot to Saved/Profiling/<name>.png. Before/after pairs MUST use identical
# args (same spawn/yaw/delay) so the only delta is the mesh.
# Usage: Scripts/capture_dragon.sh <name> "X=<n> Y=<n> Z=<n>" <yaw> [shotdelay]
# (shotdelay default 6 s; quit follows 4 s later).
set -eu
. "$(dirname "$0")/ue-env.sh"
NAME="$1"
SPAWN="$2"
YAW="$3"
SHOT="${4:-6}"
QUIT="$(awk "BEGIN { print $SHOT + 4 }")"

"$UE_BUILD" UnderTheMountain Mac Development -Project="$UPROJECT" -WaitMutex -NoHotReload -NoUba
echo "OK: UnderTheMountain (game) built - capturing $NAME ($SPAWN yaw $YAW)"

BEFORE_MARK="$(mktemp)"
"$UE_ROOT/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "$UPROJECT" /Game/Maps/Lair/Lair -game -windowed -ResX=1600 -ResY=900 \
  -ExecCmds="utm.intro.enabled 0, utm.gameflow.autostart 1, utm.proof.spawn $SPAWN, utm.proof.yaw $YAW, utm.proof.shotdelay $SHOT, utm.proof.quit $QUIT" \
  -Unattended -NoSound
SHOT_FILE="$(find Saved/Screenshots -name '*.png' -newer "$BEFORE_MARK" 2>/dev/null | head -n 1)"
rm -f "$BEFORE_MARK"
[ -n "$SHOT_FILE" ] || { echo "FAIL: no screenshot after $NAME run" >&2; exit 1; }
mkdir -p Saved/Profiling
cp "$SHOT_FILE" "Saved/Profiling/$NAME.png"
echo "OK: $NAME.png ($(stat -f%z "Saved/Profiling/$NAME.png") bytes from $SHOT_FILE)"
